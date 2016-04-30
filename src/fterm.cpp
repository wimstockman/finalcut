// File: fterm.cpp
// Provides: class FTerm

#include "fapp.h"
#include "fcharmap.h"
#include "fkey_map.h"
#include "ftcap_map.h"
#include "fterm.h"
#include "fwidget.h"
#include "fwindow.h"

#include "fonts/newfont.h"
#include "fonts/unicodemap.h"
#include "fonts/vgafont.h"

// global FTerm object
static FTerm* term_object = 0;

// global init state
static bool term_initialized = false;

// function pointer
int (*FTerm::Fputchar)(int);

// static class attributes
int      FTerm::stdin_no;
int      FTerm::stdout_no;
int      FTerm::fd_tty;
int      FTerm::x_term_pos;
int      FTerm::y_term_pos;
int      FTerm::max_color;
int      FTerm::stdin_status_flags;
uInt     FTerm::baudrate;
uInt     FTerm::tabstop;
uInt     FTerm::attr_without_color;
bool     FTerm::resize_term;
bool     FTerm::hiddenCursor;
bool     FTerm::mouse_support;
bool     FTerm::raw_mode;
bool     FTerm::input_data_pending;
bool     FTerm::terminal_update_pending;
bool     FTerm::force_terminal_update;
bool     FTerm::non_blocking_stdin;
bool     FTerm::gpm_mouse_enabled;
bool     FTerm::color256;
bool     FTerm::monochron;
bool     FTerm::background_color_erase;
bool     FTerm::automatic_left_margin;
bool     FTerm::automatic_right_margin;
bool     FTerm::eat_nl_glitch;
bool     FTerm::ansi_default_color;
bool     FTerm::xterm;
bool     FTerm::rxvt_terminal;
bool     FTerm::urxvt_terminal;
bool     FTerm::mlterm_terminal;
bool     FTerm::putty_terminal;
bool     FTerm::kde_konsole;
bool     FTerm::gnome_terminal;
bool     FTerm::kterm_terminal;
bool     FTerm::tera_terminal;
bool     FTerm::cygwin_terminal;
bool     FTerm::mintty_terminal;
bool     FTerm::linux_terminal;
bool     FTerm::screen_terminal;
bool     FTerm::tmux_terminal;
bool     FTerm::terminal_updates;
bool     FTerm::vterm_updates;
bool     FTerm::pc_charset_console;
bool     FTerm::utf8_input;
bool     FTerm::utf8_state;
bool     FTerm::utf8_console;
bool     FTerm::utf8_linux_terminal;
bool     FTerm::force_vt100;
bool     FTerm::vt100_console;
bool     FTerm::ascii_console;
bool     FTerm::NewFont;
bool     FTerm::VGAFont;
bool     FTerm::cursor_optimisation;
bool     FTerm::osc_support;
uChar    FTerm::x11_button_state;
termios  FTerm::term_init;

char     FTerm::termtype[30] = "";
char*    FTerm::term_name    = 0;
char*    FTerm::locale_name  = 0;
char*    FTerm::locale_xterm = 0;
FPoint*  FTerm::mouse        = 0;
FPoint*  FTerm::cursor       = 0;
FRect*   FTerm::term         = 0;

fc::encoding           FTerm::Encoding;
const FString*         FTerm::xterm_font         = 0;
const FString*         FTerm::xterm_title        = 0;
const FString*         FTerm::AnswerBack         = 0;
const FString*         FTerm::Sec_DA             = 0;
FOptiMove*             FTerm::opti_move          = 0;
FOptiAttr*             FTerm::opti_attr          = 0;
FTerm::term_area*      FTerm::vterm              = 0;
FTerm::term_area*      FTerm::vdesktop           = 0;
FTerm::term_area*      FTerm::vmenubar           = 0;
FTerm::term_area*      FTerm::vstatusbar         = 0;
FTerm::term_area*      FTerm::last_area          = 0;
std::queue<int>*       FTerm::output_buffer      = 0;
std::map<uChar,uChar>* FTerm::vt100_alt_char     = 0;
std::map<std::string,fc::encoding>* \
                       FTerm::encoding_set       = 0;
FOptiAttr::char_data   FTerm::term_attribute;
FOptiAttr::char_data   FTerm::next_attribute;
console_font_op        FTerm::screenFont;
unimapdesc             FTerm::screenUnicodeMap;
fc::console_cursor_style \
                       FTerm::consoleCursorStyle;


//----------------------------------------------------------------------
// class FTerm
//----------------------------------------------------------------------

// constructors and destructor
//----------------------------------------------------------------------
FTerm::FTerm()
  : map()
  , vwin(0)
{
  terminal_updates = false;
  vterm_updates    = true;

  if ( ! term_initialized )
  {
    term_initialized = true;
    term_object = this;
    fd_tty = -1;
    vterm = 0;
    vdesktop = 0;
    vmenubar = 0;
    vstatusbar = 0;
    last_area = 0;
    x_term_pos = -1;
    y_term_pos = -1;

    opti_move = new FOptiMove();
    opti_attr = new FOptiAttr();
    term      = new FRect(0,0,0,0);
    mouse     = new FPoint(0,0);
    cursor    = new FPoint(0,0);

    init();
  }
}

//----------------------------------------------------------------------
FTerm::~FTerm()  // destructor
{
  if ( term_object == this )
  {
    this->finish();
    delete cursor;
    delete mouse;
    delete term;
    delete opti_attr;
    delete opti_move;
  }
}

// private methods of FTerm
//----------------------------------------------------------------------
void FTerm::outb_Attribute_Controller (int index, int data)
{
  inb (AttrC_DataSwitch);
  outb (index & 0x1F, AttrC_Index);
  outb (uChar(data), AttrC_DataW);
  inb (AttrC_DataSwitch);
  outb (uChar((index & 0x1F) | 0x20), AttrC_Index);
  outb (uChar(data), AttrC_DataW);
}

//----------------------------------------------------------------------
int FTerm::inb_Attribute_Controller (int index)
{
  int res;
  inb (AttrC_DataSwitch);
  outb (index & 0x1F, AttrC_Index);
  res = inb (AttrC_DataR);
  inb (AttrC_DataSwitch);
  outb (uChar((index & 0x1F) | 0x20), AttrC_Index);
  inb (AttrC_DataR);
  return res;
}

//----------------------------------------------------------------------
int FTerm::getFramebuffer_bpp ()
{
  int fd = -1;
  struct fb_var_screeninfo fb_var;
  struct fb_fix_screeninfo fb_fix;

  const char* fb = const_cast<char*>("/dev/fb/0");

  if ( (fd = open(fb, O_RDWR)) < 0 )
  {
    if ( errno != ENOENT && errno != ENOTDIR )
      return -1;

    fb = const_cast<char*>("/dev/fb0");
    if ( (fd = open(fb, O_RDWR)) < 0 )
      return -1;
  }

  if (fd >= 0)
  {
    if (  ! ioctl(fd, FBIOGET_VSCREENINFO, &fb_var)
       && ! ioctl(fd, FBIOGET_FSCREENINFO, &fb_fix) )
    {
      ::close(fd);
      return int(fb_var.bits_per_pixel);
    }
    else
    {
      ::close(fd);
    }
  }
  return -1;
}

//----------------------------------------------------------------------
int FTerm::openConsole()
{
  if ( fd_tty >= 0 )  // console is already opened
    return 0;

  if ( term_name && (fd_tty = open (term_name, O_RDWR, 0)) < 0)
    if ( (fd_tty = open("/proc/self/fd/0", O_RDWR, 0)) < 0)
      if ( (fd_tty = open("/dev/tty", O_RDWR, 0)) < 0)
        if ( (fd_tty = open("/dev/tty0", O_RDWR, 0)) < 0)
          if ( (fd_tty = open("/dev/vc/0", O_RDWR, 0)) < 0)
            if ( (fd_tty = open("/dev/systty", O_RDWR, 0)) < 0)
              if ( (fd_tty = open("/dev/console", O_RDWR, 0)) < 0)
                return -1;  // No file descriptor referring to the console
  return 0;
}

//----------------------------------------------------------------------
int FTerm::closeConsole()
{
  if ( fd_tty < 0 )  // console is already closed
    return 0;

  int ret = ::close (fd_tty);  // use 'close' from the global namespace
  fd_tty = -1;
  if ( ret == 0 )
    return 0;
  else
    return -1;
}

//----------------------------------------------------------------------
int FTerm::isConsole()
{
  char arg = 0;
  // get keyboard type an compare
  return (  isatty (fd_tty)
         && ioctl(fd_tty, KDGKBTYPE, &arg) == 0
         && ((arg == KB_101) || (arg == KB_84)) );
}

//----------------------------------------------------------------------
int FTerm::getScreenFont()
{
  struct console_font_op font;
  const size_t data_size = 4 * 32 * 512;
  int ret;

  if ( fd_tty < 0 )
    return -1;

  // initialize unused padding bytes in struct
  memset(&font, 0, sizeof(console_font_op));

  font.op = KD_FONT_OP_GET;
  font.flags = 0;
  font.width = 32;
  font.height = 32;
  font.charcount = 512;
  // initialize with 0
  font.data = new uChar[data_size]();

  // font operation
  ret = ioctl (fd_tty, KDFONTOP, &font);

  if ( ret == 0 )
  {
    screenFont.width = font.width;
    screenFont.height = font.height;
    screenFont.charcount = font.charcount;
    screenFont.data = font.data;
    return 0;
  }
  else
    return -1;
}

//----------------------------------------------------------------------
int FTerm::setScreenFont ( uChar* fontdata, uInt count
                         , uInt fontwidth, uInt fontheight
                         , bool direct)
{
  struct console_font_op font;
  int ret;

  if ( fd_tty < 0 )
    return -1;

  // initialize unused padding bytes in struct
  memset(&font, 0, sizeof(console_font_op));

  font.op = KD_FONT_OP_SET;
  font.flags = 0;
  font.width = fontwidth;
  font.height = fontheight;
  font.charcount = count;

  if ( direct )
    font.data = fontdata;
  else
  {
    const uInt bytes_per_line = font.width / 8;
    const size_t data_size = bytes_per_line * 32 * font.charcount;

    try
    {
      font.data = new uChar[data_size](); // initialize with 0
    }
    catch (const std::bad_alloc& ex)
    {
      std::cerr << "not enough memory to alloc " << ex.what() << std::endl;
      return -1;
    }

    for (uInt i=0; i < count; i++)
      memcpy ( const_cast<uChar*>(font.data + bytes_per_line*32*i)
             , &fontdata[i * font.height]
             , font.height);
  }
  // font operation
  ret = ioctl (fd_tty, KDFONTOP, &font);

  if ( ret != 0 && errno != ENOSYS && errno != EINVAL )
  {
    if ( ! direct )
      delete[] font.data;
    return -1;
  }

  if ( ! direct )
    delete[] font.data;

  if ( ret == 0 )
    return 0;
  else
    return -1;
}

//----------------------------------------------------------------------
int FTerm::getUnicodeMap()
{
  struct unimapdesc unimap;
  int ret;

  if ( fd_tty < 0 )
    return -1;

  unimap.entry_ct = 0;
  unimap.entries = 0;

  // get count
  ret = ioctl (fd_tty, GIO_UNIMAP, &unimap);
  if ( ret != 0 )
  {
    int count;

    if ( errno != ENOMEM || unimap.entry_ct == 0 )
      return -1;

    count = unimap.entry_ct;

    try
    {
      unimap.entries = new struct unipair[count]();
    }
    catch (const std::bad_alloc& ex)
    {
      std::cerr << "not enough memory to alloc " << ex.what() << std::endl;
      return -1;
    }

    // get unicode-to-font mapping from kernel
    ret = ioctl(fd_tty, GIO_UNIMAP, &unimap);

    if ( ret == 0 )
    {
      screenUnicodeMap.entry_ct = unimap.entry_ct;
      screenUnicodeMap.entries = unimap.entries;
    }
    else
      return -1;
  }
  return 0;
}

//----------------------------------------------------------------------
int FTerm::setUnicodeMap (struct unimapdesc* unimap)
{
  struct unimapinit advice;
  int ret;

  if ( fd_tty < 0 )
    return -1;

  advice.advised_hashsize = 0;
  advice.advised_hashstep = 0;
  advice.advised_hashlevel = 0;

  do
  {
    // clear the unicode-to-font table
    ret = ioctl(fd_tty, PIO_UNIMAPCLR, &advice);
    if ( ret != 0 )
      return -1;

    // put the new unicode-to-font mapping in kernel
    ret = ioctl(fd_tty, PIO_UNIMAP, unimap);
    if ( ret != 0 )
      advice.advised_hashlevel++;
  }
  while ( ret != 0 && errno == ENOMEM && advice.advised_hashlevel < 100);

  if ( ret == 0 )
    return 0;
  else
    return -1;
}

//----------------------------------------------------------------------
int FTerm::setLightBackgroundColors (bool on)
{
  if ( getuid() != 0 )
    return -2;

  if ( fd_tty < 0 )
    return -1;

  // Enable access to VGA I/O ports  (from 0x3B4 with num = 0x2C)
  if ( ioctl(fd_tty, KDENABIO, 0) < 0 )
    return -1;  // error on KDENABIO

  if ( on )
    outb_Attribute_Controller (0x10, inb_Attribute_Controller(0x10) & 0xF7);
  else
    outb_Attribute_Controller (0x10, inb_Attribute_Controller(0x10) | 0x08);

  // Disable access to VGA I/O ports
  if ( ioctl(fd_tty, KDDISABIO, 0) < 0 )
    return -1;  // error on KDDISABIO

  return 0;
}

//----------------------------------------------------------------------
bool FTerm::isKeyTimeout (timeval* time, register long timeout)
{
  register long diff_usec;
  struct timeval now;
  struct timeval diff;

  FObject::getCurrentTime(now);

  diff.tv_sec = now.tv_sec - time->tv_sec;
  diff.tv_usec = now.tv_usec - time->tv_usec;
  if ( diff.tv_usec < 0 )
  {
    diff.tv_sec--;
    diff.tv_usec += 1000000;
  }
  diff_usec = (diff.tv_sec * 1000000) + diff.tv_usec;
  return (diff_usec > timeout);
}

//----------------------------------------------------------------------
int FTerm::parseKeyString ( char* buffer
                          , int buf_size
                          , timeval* time_keypressed )
{
  register uChar firstchar = uChar(buffer[0]);
  register size_t buf_len = strlen(buffer);
  const long key_timeout = 100000;  // 100 ms
  int key, len, n;

  if ( firstchar == ESC[0] )
  {
    // x11 mouse tracking
    if ( buf_len >= 6 && buffer[1] == '[' && buffer[2] == 'M' )
      return fc::Fkey_mouse;
    // SGR mouse tracking
    if (  buffer[1] == '[' && buffer[2] == '<' && buf_len >= 9
       && (buffer[buf_len-1] == 'M' || buffer[buf_len-1] == 'm') )
     return fc::Fkey_extended_mouse;
    // urxvt mouse tracking
    if (  buffer[1] == '[' && buffer[2] >= '1' && buffer[2] <= '9'
       && buffer[3] >= '0' && buffer[3] <= '9' && buf_len >= 9
       && buffer[buf_len-1] == 'M' )
     return fc::Fkey_urxvt_mouse;

    // look for termcap keys
    for (int i=0; Fkey[i].tname[0] != 0; i++)
    {
      char* k = Fkey[i].string;
      len = (k) ? int(strlen(k)) : 0;

      if ( k && strncmp(k, buffer, uInt(len)) == 0 ) // found
      {
        n = len;
        for (; n < buf_size; n++)   // Remove founded entry
          buffer[n-len] = buffer[n];
        for (; n-len < len; n++)    // Fill rest with '\0'
          buffer[n-len] = '\0';
        input_data_pending = bool(buffer[0] != '\0');
        return Fkey[i].num;
      }
    }

    // look for meta keys
    for (int i=0; Fmetakey[i].string[0] != 0; i++)
    {
      char* kmeta = Fmetakey[i].string;  // The string is never null
      len = int(strlen(kmeta));
      if ( strncmp(kmeta, buffer, uInt(len)) == 0 ) // found
      {
        if ( len == 2 && (  buffer[1] == 'O'
                         || buffer[1] == '['
                         || buffer[1] == ']') )
        {
          if ( ! isKeyTimeout(time_keypressed, key_timeout) )
            return NEED_MORE_DATA;
        }
        n = len;
        for (; n < buf_size; n++)    // Remove founded entry
          buffer[n-len] = buffer[n];
        for (; n-len < len; n++)     // Fill rest with '\0'
          buffer[n-len] = '\0';
        input_data_pending = bool(buffer[0] != '\0');
        return Fmetakey[i].num;
      }
    }
    if ( ! isKeyTimeout(time_keypressed, key_timeout) )
      return NEED_MORE_DATA;
  }

  // look for utf-8 character

  len = 1;

  if ( utf8_input && (firstchar & 0xc0) == 0xc0 )
  {
    char utf8char[4] = {};  // init array with '\0'
    if ((firstchar & 0xe0) == 0xc0)
      len = 2;
    else if ((firstchar & 0xf0) == 0xe0)
      len = 3;
    else if ((firstchar & 0xf8) == 0xf0)
      len = 4;
    for (n=0; n < len ; n++)
      utf8char[n] = char(buffer[n] & 0xff);

    key = UTF8decode(utf8char);
  }
  else
    key = uChar(buffer[0] & 0xff);

  n = len;
  for (; n < buf_size; n++)  // remove the key from the buffer front
    buffer[n-len] = buffer[n];
  for (n=n-len; n < buf_size; n++)   // fill the rest with '\0' bytes
    buffer[n] = '\0';
  input_data_pending = bool(buffer[0] != '\0');

  if ( key == 0 )  // Ctrl+Space or Ctrl+@
    key = fc::Fckey_space;

  return int(key == 127 ? fc::Fkey_backspace : key);
}

//----------------------------------------------------------------------
FString FTerm::getKeyName(int keynum)
{
  for (int i=0; FkeyName[i].string[0] != 0; i++)
    if ( FkeyName[i].num && FkeyName[i].num == keynum )
      return FString(FkeyName[i].string);
  if ( keynum > 32 && keynum < 127 )
    return FString(char(keynum));
  return FString("");
}

//----------------------------------------------------------------------
void FTerm::init_console()
{
  fd_tty = -1;
  screenUnicodeMap.entries = 0;
  screenFont.data = 0;

  if ( openConsole() == 0 )
  {
    if ( isConsole() )
    {
      getUnicodeMap();
      getScreenFont();
    }
    getTermSize();
    closeConsole();
  }
  else
  {
    std::cerr << "can not open the console.\n";
    std::abort();
  }
}

//----------------------------------------------------------------------
uInt FTerm::getBaudRate (const struct termios* termios_p)
{
  std::map<speed_t,uInt> ospeed;
  ospeed[B0]      = 0;       // hang up
  ospeed[B50]     = 50;      //      50 baud
  ospeed[B75]     = 75;      //      75 baud
  ospeed[B110]    = 110;     //     110 baud
  ospeed[B134]    = 134;     //     134.5 baud
  ospeed[B150]    = 150;     //     150 baud
  ospeed[B200]    = 200;     //     200 baud
  ospeed[B300]    = 300;     //     300 baud
  ospeed[B600]    = 600;     //     600 baud
  ospeed[B1200]   = 1200;    //   1,200 baud
  ospeed[B1800]   = 1800;    //   1,800 baud
  ospeed[B2400]   = 2400;    //   2,400 baud
  ospeed[B4800]   = 4800;    //   4,800 baud
  ospeed[B9600]   = 9600;    //   9,600 baud
  ospeed[B19200]  = 19200;   //  19,200 baud
  ospeed[B38400]  = 38400;   //  38,400 baud
  ospeed[B57600]  = 57600;   //  57,600 baud
  ospeed[B115200] = 115200;  // 115,200 baud
  ospeed[B230400] = 230400;  // 230,400 baud

  return ospeed[cfgetospeed(termios_p)];
}

//----------------------------------------------------------------------
void FTerm::init_consoleCharMap()
{
  if ( screenUnicodeMap.entry_ct != 0 )
  {
    for (int i=0; i <= lastCharItem; i++ )
    {
      bool found = false;
      for (uInt n=0; n < screenUnicodeMap.entry_ct; n++)
      {
        if ( character[i][fc::UTF8] == screenUnicodeMap.entries[n].unicode )
        {
          found = true;
          break;
        }
      }
      if ( ! found )
        character[i][fc::PC] = character[i][fc::ASCII];
    }
  }
}

//----------------------------------------------------------------------
void FTerm::signal_handler (int signum)
{
  switch (signum)
  {
    case SIGWINCH:
      if ( resize_term )
        break;
      // initialize a resize event to the root element
      resize_term = true;
      break;

    case SIGTERM:
    case SIGQUIT:
    case SIGINT:
    case SIGABRT:
    case SIGILL:
    case SIGSEGV:
      term_object->finish();
      fflush (stderr); fflush (stdout);
      fprintf ( stderr
              , "\nProgram stopped: signal %d (%s)\n"
              , signum
              , strsignal(signum) );
      std::terminate();
  }
}

//----------------------------------------------------------------------
char* FTerm::init_256colorTerminal()
{
  char local256[80] = "";
  char* new_termtype = 0;
  char *s1, *s2, *s3, *s4, *s5, *s6;

  // Enable 256 color capabilities
  s1 = getenv("COLORTERM");
  s2 = getenv("VTE_VERSION");
  s3 = getenv("XTERM_VERSION");
  s4 = getenv("ROXTERM_ID");
  s5 = getenv("KONSOLE_DBUS_SESSION");
  s6 = getenv("KONSOLE_DCOP");

  if ( s1 != 0 )
    strncat (local256, s1, sizeof(local256) - strlen(local256) - 1);
  if ( s2 != 0 )
    strncat (local256, s2, sizeof(local256) - strlen(local256) - 1);
  if ( s3 != 0 )
    strncat (local256, s3, sizeof(local256) - strlen(local256) - 1);
  if ( s4 != 0 )
    strncat (local256, s4, sizeof(local256) - strlen(local256) - 1);
  if ( s5 != 0 )
    strncat (local256, s5, sizeof(local256) - strlen(local256) - 1);
  if ( s6 != 0 )
    strncat (local256, s6, sizeof(local256) - strlen(local256) - 1);

  if ( strlen(local256) > 0 )
  {
    if ( strncmp(termtype, "xterm", 5) == 0 )
      new_termtype = const_cast<char*>("xterm-256color");

    if ( strncmp(termtype, "screen", 6) == 0 )
    {
      new_termtype = const_cast<char*>("screen-256color");
      screen_terminal = true;

      char* tmux = getenv("TMUX");
      if ( tmux && strlen(tmux) != 0 )
        tmux_terminal = true;
    }

    if ( strncmp(termtype, "Eterm", 5) == 0 )
      new_termtype = const_cast<char*>("Eterm-256color");

    if ( strncmp(termtype, "mlterm", 6) == 0 )
    {
      new_termtype = const_cast<char*>("mlterm-256color");
      mlterm_terminal = true;
    }
    if ( strncmp(termtype, "rxvt", 4) != 0
       && s1
       && strncmp(s1, "rxvt-xpm", 8) == 0 )
    {
      new_termtype = const_cast<char*>("rxvt-256color");
      rxvt_terminal = true;
    }
    color256 = true;
  }
  else if ( strstr (termtype, "256color") )
    color256 = true;
  else
    color256 = false;

  if ( (s5 && strlen(s5) > 0) || (s6 && strlen(s6) > 0) )
    kde_konsole = true;

  if ( (s1 && strncmp(s1, "gnome-terminal", 14) == 0) || s2 )
  {
    if ( color256 )
      new_termtype = const_cast<char*>("gnome-256color");
    else
      new_termtype = const_cast<char*>("gnome");
    gnome_terminal = true;
  }

  return new_termtype;
}

//----------------------------------------------------------------------
char* FTerm::parseAnswerbackMsg (char*& current_termtype)
{
  char* new_termtype = current_termtype;

  // send ENQ and read the answerback message
  AnswerBack = new FString(getAnswerbackMsg());

  if ( AnswerBack && *AnswerBack == FString("PuTTY") )
  {
    putty_terminal = true;
    if ( color256 )
      new_termtype = const_cast<char*>("putty-256color");
    else
      new_termtype = const_cast<char*>("putty");
  }
  else
    putty_terminal = false;

  if ( cygwin_terminal )
    putchar (BS[0]);  // cygwin needs a backspace to delete the '♣' char

  return new_termtype;
}

//----------------------------------------------------------------------
char* FTerm::parseSecDA (char*& current_termtype)
{
  char* new_termtype = current_termtype;
  bool  sec_da_supported = false;


  // The Linux console knows no Sec_DA
  if ( linux_terminal )
    return new_termtype;

  // secondary device attributes (SEC_DA) <- decTerminalID string
  Sec_DA = new FString(getSecDA());

  if ( Sec_DA && Sec_DA->getLength() > 5 )
  {
    uLong num_components;

    // remove the first 3 bytes ("\033[>")
    FString temp = Sec_DA->right(Sec_DA->getLength() - 3);
    // remove the last byte ("c")
    temp.remove(temp.getLength()-1, 1);
    // split into components
    std::vector<FString> Sec_DA_split = temp.split(';');

    num_components = Sec_DA_split.size();

    if ( num_components == 3 )
      sec_da_supported = true;

    if ( num_components >= 2 )
    {
      FString* Sec_DA_components = &Sec_DA_split[0];

      if ( ! Sec_DA_components[0].isEmpty() )
      {
        int terminal_id_type, terminal_id_version;

        // Read the terminal type
        try
        {
          terminal_id_type = Sec_DA_components[0].toInt();
        }
        catch (const std::exception&)
        {
          terminal_id_type = -1;
        }

        // Read the terminal (firmware) version
        try
        {
          if ( Sec_DA_components[1] )
            terminal_id_version = Sec_DA_components[1].toInt();
          else
            terminal_id_version = -1;
        }
        catch (const std::exception&)
        {
          terminal_id_version = -1;
        }

        switch ( terminal_id_type )
        {
          case 0:
            if ( terminal_id_version == 115 )
              kde_konsole = true;
            else if ( terminal_id_version == 136 )
              putty_terminal = true;  // PuTTY
            break;

          case 1:
            if ( ! sec_da_supported )
            {
              if ( terminal_id_version ==  2 )  // also used by apple terminal
                kterm_terminal = true;  // kterm
            }
            else if ( terminal_id_version > 1000 )
            {
              gnome_terminal = true;  // vte / gnome terminal
              if ( color256 )
                new_termtype = const_cast<char*>("gnome-256color");
              else
                new_termtype = const_cast<char*>("gnome");
            }
            break;

          case 32:  // Tera Term
            tera_terminal = true;
            new_termtype = const_cast<char*>("teraterm");
            break;

          case 77:  // mintty
            mintty_terminal = true;
            new_termtype = const_cast<char*>("xterm-256color");
            // application escape key mode
            putstring (CSI "?7727h");
            fflush(stdout);
            break;

          case 83:  // screen
            screen_terminal = true;
            break;

          case 82:  // rxvt
            rxvt_terminal = true;
            force_vt100 = true;  // this rxvt terminal support on utf-8
            if (  strncmp(termtype, "rxvt-", 5) != 0
               || strncmp(termtype, "rxvt-cygwin-native", 5) == 0 )
              new_termtype = const_cast<char*>("rxvt-16color");
            break;

          case 85:  // rxvt-unicode
            rxvt_terminal = true;
            urxvt_terminal = true;
            if ( strncmp(termtype, "rxvt-", 5) != 0 )
            {
              if ( color256 )
                new_termtype = const_cast<char*>("rxvt-256color");
              else
                new_termtype = const_cast<char*>("rxvt");
            }
            break;

          default:
            break;
        }
      }
    }
  }
  return new_termtype;
}

//----------------------------------------------------------------------
void FTerm::oscPrefix()
{
  if ( tmux_terminal )
  {
    // tmux device control string
    putstring (ESC "Ptmux;" ESC);
  }
  else if ( screen_terminal )
  {
    // GNU Screen device control string
    putstring (ESC "P");
  }
}

//----------------------------------------------------------------------
void FTerm::oscPostfix()
{
  if ( screen_terminal || tmux_terminal )
  {
    // GNU Screen/tmux string terminator
    putstring (ESC "\\");
  }
}

//----------------------------------------------------------------------
void FTerm::init_alt_charset()
{
  // read the used vt100 pairs
  if ( tcap[t_acs_chars].string )
  {
    for (int n=0; tcap[t_acs_chars].string[n]; tcap[t_acs_chars].string += 2)
    {
      // insert the vt100 key/value pairs into a map
      uChar p1 = uChar(tcap[t_acs_chars].string[n]);
      uChar p2 = uChar(tcap[t_acs_chars].string[n+1]);
      (*vt100_alt_char)[p1] = p2;
    }
  }

  enum column
  {
    vt100_key = 0,
    utf8_char = 1
  };

  // update array 'character' with discovered vt100 pairs
  for (int n=0; n <= lastKeyItem; n++ )
  {
    uChar keyChar = uChar(vt100_key_to_utf8[n][vt100_key]);
    uChar altChar = uChar((*vt100_alt_char)[ keyChar ]);
    uInt utf8char = uInt(vt100_key_to_utf8[n][utf8_char]);
    fc::encoding num = fc::NUM_OF_ENCODINGS;

    uInt* p = std::find ( character[0]
                        , character[lastCharItem] + num
                        , utf8char );
    if ( p != character[lastCharItem] + num ) // found in character
    {
      int item = int(std::distance(character[0], p) / num);

      if ( altChar )
        character[item][fc::VT100] = altChar; // update alternate character set
      else
        character[item][fc::VT100] = 0; // delete vt100 char in character
    }
  }
}

//----------------------------------------------------------------------
void FTerm::init_pc_charset()
{
  bool reinit = false;

  // rxvt does not support pc charset
  if ( rxvt_terminal || urxvt_terminal )
    return;

  // fallback if "S2" is not found
  if ( ! tcap[t_enter_pc_charset_mode].string )
  {
    if ( utf8_console )
    {
      // Select iso8859-1 + null mapping
      tcap[t_enter_pc_charset_mode].string = \
        const_cast<char*>(ESC "%@" ESC "(U");
    }
    else
    {
      // Select null mapping
      tcap[t_enter_pc_charset_mode].string = \
        const_cast<char*>(ESC "(U");
    }
    opti_attr->set_enter_pc_charset_mode (tcap[t_enter_pc_charset_mode].string);
    reinit = true;
  }

  // fallback if "S3" is not found
  if ( ! tcap[t_exit_pc_charset_mode].string )
  {
    if ( utf8_console )
    {
      // Select ascii mapping + utf8
      tcap[t_exit_pc_charset_mode].string = \
        const_cast<char*>(ESC "(B" ESC "%G");
    }
    else
    {
      // Select ascii mapping
      tcap[t_enter_pc_charset_mode].string = \
        const_cast<char*>(ESC "(B");
    }
    opti_attr->set_exit_pc_charset_mode (tcap[t_exit_pc_charset_mode].string);
    reinit = true;
  }

  if ( reinit )
    opti_attr->init();
}

//----------------------------------------------------------------------
void FTerm::init_termcaps()
{
  /* Terminal capability data base
   * -----------------------------
   * Info under: man 5 terminfo
   *
   * Importent shell commands:
   *   captoinfo - convert all termcap descriptions into terminfo descriptions
   *   infocmp   - print out terminfo description from the current terminal
   */
  static char term_buffer[2048];
  static char string_buf[2048];
  char* buffer = string_buf;

  // open the termcap file + load entry for termtype
  if ( tgetent(term_buffer, termtype) == 1 )
  {
    char* key_up_string;

    // screen erased with the background color
    background_color_erase = tgetflag(const_cast<char*>("ut"));

    // t_cursor_left wraps from column 0 to last column
    automatic_left_margin = tgetflag(const_cast<char*>("bw"));

    // terminal has auto-matic margins
    automatic_right_margin = tgetflag(const_cast<char*>("am"));

    // newline ignored after 80 cols
    eat_nl_glitch = tgetflag(const_cast<char*>("xn"));

    // terminal supports ANSI set default fg and bg color
    ansi_default_color = tgetflag(const_cast<char*>("AX"));

    // terminal supports operating system commands (OSC)
    // OSC = Esc + ']'
    osc_support = tgetflag(const_cast<char*>("XT"));

    if ( isTeraTerm() )
      eat_nl_glitch = true;

    // maximum number of colors on screen
    max_color = tgetnum(const_cast<char*>("Co"));

    if ( max_color < 0 )
      max_color = 1;

    if ( max_color < 8 )
      monochron = true;
    else
      monochron = false;

    tabstop = uInt(tgetnum(const_cast<char*>("it")));
    attr_without_color = uInt(tgetnum(const_cast<char*>("NC")));

    // gnome-terminal has NC=16 however, it can use the dim attribute
    if ( gnome_terminal )
      attr_without_color = 0;
    // PuTTY has NC=22 however, it can show underline and reverse
    if ( putty_terminal )
      attr_without_color = 16;

    // read termcap output strings
    for (int i=0; tcap[i].tname[0] != 0; i++)
      tcap[i].string = tgetstr(tcap[i].tname, &buffer);

    // set invisible cursor for cygwin terminal
    if ( cygwin_terminal && ! tcap[t_cursor_invisible].string )
      tcap[t_cursor_invisible].string = \
        const_cast<char*>(CSI "?25l");

    // set visible cursor for cygwin terminal
    if ( cygwin_terminal && ! tcap[t_cursor_visible].string )
      tcap[t_cursor_visible].string = \
        const_cast<char*>(CSI "?25h");

    // set ansi blink for cygwin terminal
    if ( cygwin_terminal && ! tcap[t_enter_blink_mode].string )
      tcap[t_enter_blink_mode].string = \
        const_cast<char*>(CSI "5m");

    // set enter/exit alternative charset mode for rxvt terminal
    if ( rxvt_terminal && strncmp(termtype, "rxvt-16color", 12) == 0 )
    {
      tcap[t_enter_alt_charset_mode].string = \
        const_cast<char*>(ESC "(0");
      tcap[t_exit_alt_charset_mode].string  = \
        const_cast<char*>(ESC "(B");
    }

    // set exit underline for gnome terminal
    if ( gnome_terminal )
      tcap[t_exit_underline_mode].string = \
        const_cast<char*>(CSI "24m");

    // set ansi foreground and background color
    if ( linux_terminal || cygwin_terminal )
    {
      tcap[t_set_a_foreground].string = \
        const_cast<char*>(CSI "3%p1%{8}%m%d%?%p1%{7}%>%t;1%e;21%;m");
      tcap[t_set_a_background].string = \
        const_cast<char*>(CSI "4%p1%{8}%m%d%?%p1%{7}%>%t;5%e;25%;m");
      tcap[t_orig_pair].string = \
        const_cast<char*>(CSI "39;49;25m");
    }
    else if ( rxvt_terminal && ! urxvt_terminal )
    {
      tcap[t_set_a_foreground].string = \
        const_cast<char*>(CSI "%?%p1%{8}%<%t%p1%{30}%+%e%p1%'R'%+%;%dm");
      tcap[t_set_a_background].string = \
        const_cast<char*>(CSI "%?%p1%{8}%<%t%p1%'('%+%e%p1%{92}%+%;%dm");
    }
    else if ( tera_terminal )
    {
      tcap[t_set_a_foreground].string = \
        const_cast<char*>(CSI "38;5;%p1%dm");
      tcap[t_set_a_background].string = \
        const_cast<char*>(CSI "48;5;%p1%dm");
      tcap[t_exit_attribute_mode].string = \
        const_cast<char*>(CSI "0m" SI);
    }
    else if ( putty_terminal )
    {
      tcap[t_set_a_foreground].string = \
        const_cast<char*>(CSI "%?%p1%{8}%<"
                          "%t3%p1%d"
                          "%e%p1%{16}%<"
                          "%t9%p1%{8}%-%d"
                          "%e38;5;%p1%d%;m");
      tcap[t_set_a_background].string = \
        const_cast<char*>(CSI "%?%p1%{8}%<"
                          "%t4%p1%d"
                          "%e%p1%{16}%<"
                          "%t10%p1%{8}%-%d"
                          "%e48;5;%p1%d%;m");
    }

    // fallback if "AF" is not found
    if ( ! tcap[t_set_a_foreground].string )
      tcap[t_set_a_foreground].string = \
        const_cast<char*>(CSI "3%p1%dm");

    // fallback if "AB" is not found
    if ( ! tcap[t_set_a_background].string )
      tcap[t_set_a_background].string = \
        const_cast<char*>(CSI "4%p1%dm");

    // fallback if "Ic" is not found
    if ( ! tcap[t_initialize_color].string )
    {
      if ( screen_terminal )
      {
        if ( tmux_terminal )
        {
          tcap[t_initialize_color].string = \
            const_cast<char*>(ESC "Ptmux;" ESC OSC "4;%p1%d;rgb:"
                              "%p2%{255}%*%{1000}%/%2.2X/"
                              "%p3%{255}%*%{1000}%/%2.2X/"
                              "%p4%{255}%*%{1000}%/%2.2X" BEL ESC "\\");
        }
        else
        {
          tcap[t_initialize_color].string = \
            const_cast<char*>(ESC "P" OSC "4;%p1%d;rgb:"
                              "%p2%{255}%*%{1000}%/%2.2X/"
                              "%p3%{255}%*%{1000}%/%2.2X/"
                              "%p4%{255}%*%{1000}%/%2.2X" BEL ESC "\\");
        }
      }
      else
      {
        tcap[t_initialize_color].string = \
          const_cast<char*>(OSC "P%p1%x"
                            "%p2%{255}%*%{1000}%/%02x"
                            "%p3%{255}%*%{1000}%/%02x"
                            "%p4%{255}%*%{1000}%/%02x");
      }
   }

    // fallback if "ti" is not found
    if ( ! tcap[t_enter_ca_mode].string )
      tcap[t_enter_ca_mode].string = \
        const_cast<char*>(ESC "7" CSI "?47h");

    // fallback if "te" is not found
    if ( ! tcap[t_exit_ca_mode].string )
      tcap[t_exit_ca_mode].string = \
        const_cast<char*>(CSI "?47l" ESC "8" CSI "m");

    // set ansi move if "cm" is not found
    if ( ! tcap[t_cursor_address].string )
      tcap[t_cursor_address].string = \
        const_cast<char*>(CSI "%i%p1%d;%p2%dH");

    // test for standard ECMA-48 (ANSI X3.64) terminal
    if ( tcap[t_exit_underline_mode].string
       && strncmp(tcap[t_exit_underline_mode].string, CSI "24m", 5) == 0 )
    {
      // seems to be a ECMA-48 (ANSI X3.64) compatible terminal
      tcap[t_enter_dbl_underline_mode].string = \
        const_cast<char*>(CSI "21m");  // Exit single underline, too

      tcap[t_exit_dbl_underline_mode].string = \
        const_cast<char*>(CSI "24m");

      tcap[t_exit_bold_mode].string = \
        const_cast<char*>(CSI "22m");  // Exit dim, too

      tcap[t_exit_dim_mode].string = \
        const_cast<char*>(CSI "22m");

      tcap[t_exit_underline_mode].string = \
        const_cast<char*>(CSI "24m");

      tcap[t_exit_blink_mode].string = \
        const_cast<char*>(CSI "25m");

      tcap[t_exit_reverse_mode].string = \
        const_cast<char*>(CSI "27m");

      tcap[t_exit_secure_mode].string = \
        const_cast<char*>(CSI "28m");

      tcap[t_enter_crossed_out_mode].string = \
        const_cast<char*>(CSI "9m");

      tcap[t_exit_crossed_out_mode].string = \
        const_cast<char*>(CSI "29m");
    }

    // read termcap key strings
    for (int i=0; Fkey[i].tname[0] != 0; i++)
    {
      Fkey[i].string = tgetstr(Fkey[i].tname, &buffer);

      // fallback for rxvt with TERM=xterm
      if ( strncmp(Fkey[i].tname, "khx", 3) == 0 )
        Fkey[i].string = const_cast<char*>(CSI "7~");  // home key

      if ( strncmp(Fkey[i].tname, "@7x", 3) == 0 )
        Fkey[i].string = const_cast<char*>(CSI "8~");  // end key

      if ( strncmp(Fkey[i].tname, "k1x", 3) == 0 )
        Fkey[i].string = const_cast<char*>(CSI "11~");  // F1

      if ( strncmp(Fkey[i].tname, "k2x", 3) == 0 )
        Fkey[i].string = const_cast<char*>(CSI "12~");  // F2

      if ( strncmp(Fkey[i].tname, "k3x", 3) == 0 )
        Fkey[i].string = const_cast<char*>(CSI "13~");  // F3

      if ( strncmp(Fkey[i].tname, "k4x", 3) == 0 )
        Fkey[i].string = const_cast<char*>(CSI "14~");  // F4

      // fallback for TERM=ansi
      if ( strncmp(Fkey[i].tname, "@7X", 3) == 0 )
        Fkey[i].string = const_cast<char*>(CSI "K");  // end key
    }

    // Some terminals (e.g. PuTTY) send the wrong code for the arrow keys
    // http://www.unix.com/shell-programming-scripting/..
    //      ..110380-using-arrow-keys-shell-scripts.html
    key_up_string = tgetstr(const_cast<char*>("ku"), &buffer);
    if (  (key_up_string && (strcmp(key_up_string, CSI "A") == 0))
       || (  tcap[t_cursor_up].string
          && (strcmp(tcap[t_cursor_up].string, CSI "A") == 0)) )
    {
      for (int i=0; Fkey[i].tname[0] != 0; i++)
      {
        if ( strncmp(Fkey[i].tname, "kux", 3) == 0 )
          Fkey[i].string = const_cast<char*>(CSI "A");  // key up

        if ( strncmp(Fkey[i].tname, "kdx", 3) == 0 )
          Fkey[i].string = const_cast<char*>(CSI "B");  // key down

        if ( strncmp(Fkey[i].tname, "krx", 3) == 0 )
          Fkey[i].string = const_cast<char*>(CSI "C");  // key right

        if ( strncmp(Fkey[i].tname, "klx", 3) == 0 )
          Fkey[i].string = const_cast<char*>(CSI "D");  // key left

        if ( strncmp(Fkey[i].tname, "k1X", 3) == 0 )
          Fkey[i].string = const_cast<char*>(ESC "OP");  // PF1

        if ( strncmp(Fkey[i].tname, "k2X", 3) == 0 )
          Fkey[i].string = const_cast<char*>(ESC "OQ");  // PF2

        if ( strncmp(Fkey[i].tname, "k3X", 3) == 0 )
          Fkey[i].string = const_cast<char*>(ESC "OR");  // PF3

        if ( strncmp(Fkey[i].tname, "k4X", 3) == 0 )
          Fkey[i].string = const_cast<char*>(ESC "OS");  // PF4
      }
    }
  }
  else
  {
    std::cerr << "Unknown terminal: "  << termtype << "\n"
              << "Check the TERM environment variable\n"
              << "Also make sure that the terminal"
              << "is defined in the terminfo database.\n";
    std::abort();
  }

  // duration precalculation of the cursor movement strings
  opti_move->setTabStop(int(tabstop));
  opti_move->set_cursor_home (tcap[t_cursor_home].string);
  opti_move->set_cursor_to_ll (tcap[t_cursor_to_ll].string);
  opti_move->set_carriage_return (tcap[t_carriage_return].string);
  opti_move->set_tabular (tcap[t_tab].string);
  opti_move->set_back_tab (tcap[t_back_tab].string);
  opti_move->set_cursor_up (tcap[t_cursor_up].string);
  opti_move->set_cursor_down (tcap[t_cursor_down].string);
  opti_move->set_cursor_left (tcap[t_cursor_left].string);
  opti_move->set_cursor_right (tcap[t_cursor_right].string);
  opti_move->set_cursor_address (tcap[t_cursor_address].string);
  opti_move->set_column_address (tcap[t_column_address].string);
  opti_move->set_row_address (tcap[t_row_address].string);
  opti_move->set_parm_up_cursor (tcap[t_parm_up_cursor].string);
  opti_move->set_parm_down_cursor (tcap[t_parm_down_cursor].string);
  opti_move->set_parm_left_cursor (tcap[t_parm_left_cursor].string);
  opti_move->set_parm_right_cursor (tcap[t_parm_right_cursor].string);
  opti_move->set_auto_left_margin (automatic_left_margin);
  opti_move->set_eat_newline_glitch (eat_nl_glitch);
  //opti_move->printDurations();

  // attribute settings
  opti_attr->setNoColorVideo (int(attr_without_color));
  opti_attr->set_enter_bold_mode (tcap[t_enter_bold_mode].string);
  opti_attr->set_exit_bold_mode (tcap[t_exit_bold_mode].string);
  opti_attr->set_enter_dim_mode (tcap[t_enter_dim_mode].string);
  opti_attr->set_exit_dim_mode (tcap[t_exit_dim_mode].string);
  opti_attr->set_enter_italics_mode (tcap[t_enter_italics_mode].string);
  opti_attr->set_exit_italics_mode (tcap[t_exit_italics_mode].string);
  opti_attr->set_enter_underline_mode (tcap[t_enter_underline_mode].string);
  opti_attr->set_exit_underline_mode (tcap[t_exit_underline_mode].string);
  opti_attr->set_enter_blink_mode (tcap[t_enter_blink_mode].string);
  opti_attr->set_exit_blink_mode (tcap[t_exit_blink_mode].string);
  opti_attr->set_enter_reverse_mode (tcap[t_enter_reverse_mode].string);
  opti_attr->set_exit_reverse_mode (tcap[t_exit_reverse_mode].string);
  opti_attr->set_enter_standout_mode (tcap[t_enter_standout_mode].string);
  opti_attr->set_exit_standout_mode (tcap[t_exit_standout_mode].string);
  opti_attr->set_enter_secure_mode (tcap[t_enter_secure_mode].string);
  opti_attr->set_exit_secure_mode (tcap[t_exit_secure_mode].string);
  opti_attr->set_enter_protected_mode (tcap[t_enter_protected_mode].string);
  opti_attr->set_exit_protected_mode (tcap[t_exit_protected_mode].string);
  opti_attr->set_enter_crossed_out_mode (tcap[t_enter_crossed_out_mode].string);
  opti_attr->set_exit_crossed_out_mode (tcap[t_exit_crossed_out_mode].string);
  opti_attr->set_enter_dbl_underline_mode (tcap[t_enter_dbl_underline_mode].string);
  opti_attr->set_exit_dbl_underline_mode (tcap[t_exit_dbl_underline_mode].string);
  opti_attr->set_set_attributes (tcap[t_set_attributes].string);
  opti_attr->set_exit_attribute_mode (tcap[t_exit_attribute_mode].string);
  opti_attr->set_enter_alt_charset_mode (tcap[t_enter_alt_charset_mode].string);
  opti_attr->set_exit_alt_charset_mode (tcap[t_exit_alt_charset_mode].string);
  opti_attr->set_enter_pc_charset_mode (tcap[t_enter_pc_charset_mode].string);
  opti_attr->set_exit_pc_charset_mode (tcap[t_exit_pc_charset_mode].string);
  opti_attr->set_a_foreground_color (tcap[t_set_a_foreground].string);
  opti_attr->set_a_background_color (tcap[t_set_a_background].string);
  opti_attr->set_foreground_color (tcap[t_set_foreground].string);
  opti_attr->set_background_color (tcap[t_set_background].string);
  opti_attr->set_term_color_pair (tcap[t_set_color_pair].string);
  opti_attr->set_orig_pair (tcap[t_orig_pair].string);
  opti_attr->set_orig_orig_colors (tcap[t_orig_colors].string);
  opti_attr->setMaxColor (max_color);
  if ( ansi_default_color )
    opti_attr->setDefaultColorSupport();
  if ( cygwin_terminal )
    opti_attr->setCygwinTerminal();
  opti_attr->init();
}

//----------------------------------------------------------------------
void FTerm::init_encoding()
{
  if ( isatty(stdout_no) && ! strcmp(nl_langinfo(CODESET), "UTF-8") )
  {
    utf8_console = true;
    Encoding = fc::UTF8;
    Fputchar = &FTerm::putchar_UTF8;  // function pointer
    utf8_state = true;
    utf8_input = true;
    setUTF8(true);
  }
  else if (  isatty(stdout_no)
          && (strlen(termtype) > 0)
          && (tcap[t_exit_alt_charset_mode].string != 0) )
  {
    vt100_console = true;
    Encoding = fc::VT100;
    Fputchar = &FTerm::putchar_ASCII;  // function pointer
  }
  else
  {
    ascii_console = true;
    Encoding = fc::ASCII;
    Fputchar = &FTerm::putchar_ASCII;  // function pointer
  }

  init_pc_charset();

  if (  linux_terminal
     || cygwin_terminal
     || NewFont
     || (putty_terminal && ! utf8_state) )
  {
    pc_charset_console = true;
    Encoding = fc::PC;
    Fputchar = &FTerm::putchar_ASCII;  // function pointer

    if ( linux_terminal && utf8_console )
    {
      utf8_linux_terminal = true;
      setUTF8(false);
    }
    else if ( xterm && utf8_console )
    {
      Fputchar = &FTerm::putchar_UTF8;  // function pointer
    }
  }

  if ( force_vt100 )
  {
    vt100_console = true;
    Encoding = fc::VT100;
    Fputchar = &FTerm::putchar_ASCII;  // function pointer
  }
}

//----------------------------------------------------------------------
void FTerm::init()
{
  char* new_termtype = 0;
  x11_button_state = 0x03;
  max_color = 1;

  output_buffer  = new std::queue<int>;
  vt100_alt_char = new std::map<uChar,uChar>;
  encoding_set   = new std::map<std::string,fc::encoding>;

  // Define the encoding set
  (*encoding_set)["UTF8"]  = fc::UTF8;
  (*encoding_set)["UTF-8"] = fc::UTF8;
  (*encoding_set)["VT100"] = fc::VT100;
  (*encoding_set)["PC"]    = fc::PC;
  (*encoding_set)["ASCII"] = fc::ASCII;

  // Preset to false
  utf8_console           = \
  utf8_input             = \
  utf8_state             = \
  utf8_linux_terminal    = \
  pc_charset_console     = \
  vt100_console          = \
  NewFont                = \
  VGAFont                = \
  ascii_console          = \
  hiddenCursor           = \
  mouse_support          = \
  force_vt100            = \
  tera_terminal          = \
  kterm_terminal         = \
  gnome_terminal         = \
  kde_konsole            = \
  rxvt_terminal          = \
  urxvt_terminal         = \
  mlterm_terminal        = \
  mintty_terminal        = \
  screen_terminal        = \
  tmux_terminal          = \
  background_color_erase = false;

  // term_attribute stores the current state of the terminal
  term_attribute.code          = '\0';
  term_attribute.fg_color      = fc::Default;
  term_attribute.bg_color      = fc::Default;
  term_attribute.bold          = \
  term_attribute.dim           = \
  term_attribute.italic        = \
  term_attribute.underline     = \
  term_attribute.blink         = \
  term_attribute.reverse       = \
  term_attribute.standout      = \
  term_attribute.invisible     = \
  term_attribute.protect       = \
  term_attribute.crossed_out   = \
  term_attribute.dbl_underline = \
  term_attribute.alt_charset   = \
  term_attribute.pc_charset    = false;

  // next_attribute contains the state of the next printed character
  next_attribute.code          = '\0';
  next_attribute.fg_color      = fc::Default;
  next_attribute.bg_color      = fc::Default;
  next_attribute.bold          = \
  next_attribute.dim           = \
  next_attribute.italic        = \
  next_attribute.underline     = \
  next_attribute.blink         = \
  next_attribute.reverse       = \
  next_attribute.standout      = \
  next_attribute.invisible     = \
  next_attribute.protect       = \
  next_attribute.crossed_out   = \
  next_attribute.dbl_underline = \
  next_attribute.alt_charset   = \
  next_attribute.pc_charset    = false;

  // Preset to true
  cursor_optimisation    = true;

  // assertion: programm start in cooked mode
  raw_mode                = \
  input_data_pending      = \
  terminal_update_pending = \
  force_terminal_update   = \
  non_blocking_stdin      = false;

  stdin_no  = fileno(stdin);
  stdout_no = fileno(stdout);
  stdin_status_flags = fcntl(stdin_no, F_GETFL);

  if ( stdin_status_flags == -1 )
    std::abort();

  term_name = ttyname(stdout_no);
  // -> possible fallback: look into /etc/ttytype for the type?

  // initialize terminal and Linux console
  init_console();

  // create virtual terminal
  createVTerm();

  // create virtual desktop area
  createArea (vdesktop);
  vdesktop->visible = true;

  // make stdin non-blocking
  setNonBlockingInput();

  // save termios settings
  tcgetattr (stdin_no, &term_init);

  // get output baud rate
  baudrate = getBaudRate(&term_init);

  if ( isatty(stdout_no) )
    opti_move->setBaudRate(int(baudrate));

  // Import the untrusted environment variable TERM
  const char* term_env = getenv(const_cast<char*>("TERM"));
  if ( term_env )
    strncpy (termtype, term_env, sizeof(termtype) - 1);
  else
    strncpy (termtype, const_cast<char*>("vt100"), 6);

  // initialize 256 colors terminals
  new_termtype = init_256colorTerminal();

  if ( strncmp(termtype, "cygwin", 6) == 0 )
    cygwin_terminal = true;
  else
    cygwin_terminal = false;

  if ( strncmp(termtype, "rxvt-cygwin-native", 18) == 0 )
    rxvt_terminal = true;

  // Test for Linux console
  if (  strncmp(termtype, const_cast<char*>("linux"), 5) == 0
     || strncmp(termtype, const_cast<char*>("con"), 3) == 0 )
    linux_terminal = true;
  else
    linux_terminal = false;

  // terminal detection...
  setRawMode();

  new_termtype = parseAnswerbackMsg (new_termtype);
  new_termtype = parseSecDA (new_termtype);

  unsetRawMode();
  // ...end of terminal detection

  // stop non-blocking stdin
  unsetNonBlockingInput();

  // Test if the terminal is a xterm
  if (  strncmp(termtype, const_cast<char*>("xterm"), 5) == 0
     || strncmp(termtype, const_cast<char*>("Eterm"), 4) == 0 )
    xterm = true;
  else
    xterm = false;

  // set the new environment variable TERM
  if ( new_termtype )
  {
    setenv(const_cast<char*>("TERM"), new_termtype, 1);
    strncpy (termtype, new_termtype, strlen(new_termtype)+1);
  }

  // Initializes variables for the current terminal
  init_termcaps();
  init_alt_charset();

  // set the Fputchar function pointer
  locale_name = setlocale(LC_ALL, ""); // init current locale

  // get XTERM_LOCALE
  locale_xterm = getenv("XTERM_LOCALE");

  // set LC_ALL to XTERM_LOCALE
  if ( locale_xterm )
    locale_name = setlocale(LC_ALL, locale_xterm);

  // TeraTerm can not show UTF-8 character
  if ( tera_terminal && ! strcmp(nl_langinfo(CODESET), "UTF-8") )
    locale_name = setlocale(LC_ALL, "en_US");

  // if locale C => switch from 7bit ascii -> latin1
  if ( isatty(stdout_no) && ! strcmp(nl_langinfo(CODESET), "ANSI_X3.4-1968") )
    locale_name = setlocale(LC_ALL, "en_US");

  // try to found a meaningful content for locale_name
  if ( locale_name )
    locale_name = setlocale(LC_CTYPE, 0);
  else
  {
    locale_name = getenv("LC_ALL");
    if ( ! locale_name )
    {
      locale_name = getenv("LC_CTYPE");
      if ( ! locale_name )
        locale_name = getenv("LANG");
    }
  }
  // Fallback to C
  if ( ! locale_name )
    locale_name = const_cast<char*>("C");

  // Detect environment and set encoding
  init_encoding();

#ifdef F_HAVE_LIBGPM
  // Enable the linux general purpose mouse (gpm) server
  gpm_mouse_enabled = enableGpmMouse();
#endif

  // xterm mouse support
  if ( tcap[t_key_mouse].string != 0 )
  {
    mouse_support = true;
    enableXTermMouse();
  }

  // enter 'keyboard_transmit' mode
  if ( tcap[t_keypad_xmit].string )
  {
    putstring (tcap[t_keypad_xmit].string);
    fflush(stdout);
  }

  // save current cursor position
  if ( tcap[t_save_cursor].string )
  {
    putstring (tcap[t_save_cursor].string);
    fflush(stdout);
  }

  // saves the screen and the cursor position
  if ( tcap[t_enter_ca_mode].string )
  {
    putstring (tcap[t_enter_ca_mode].string);
    fflush(stdout);
  }

  // enable alternate charset
  if ( tcap[t_enable_acs].string )
  {
    putstring (tcap[t_enable_acs].string);
    fflush(stdout);
  }

  setXTermCursorStyle(fc::blinking_underline);
  setXTermMouseBackground("rgb:ffff/ffff/ffff");
  setXTermMouseForeground ("rgb:0000/0000/0000");
  if ( ! gnome_terminal )
    setXTermCursorColor("rgb:ffff/ffff/ffff");
  if ( ! mintty_terminal && ! rxvt_terminal && ! screen_terminal )
  {
    // mintty and rxvt can't reset these settings
    setXTermBackground("rgb:8080/a4a4/ecec");
    setXTermForeground("rgb:0000/0000/0000");
    setXTermHighlightBackground("rgb:8686/8686/8686");
  }

  setRawMode();
  hideCursor();

  if ( (xterm || urxvt_terminal) && ! rxvt_terminal )
  {
    setNonBlockingInput();
    xterm_font  = new FString(getXTermFont());
    xterm_title = new FString(getXTermTitle());
    unsetNonBlockingInput();
  }

  if ( (max_color == 8)
     && (  linux_terminal
        || cygwin_terminal
        || putty_terminal
        || tera_terminal
        || rxvt_terminal) )
  {
    max_color = 16;
  }

  if ( linux_terminal && openConsole() == 0 )
  {
    if ( isConsole() )
      if ( setLightBackgroundColors(true) != 0 )
        max_color = 8;
    closeConsole();
    setConsoleCursor(fc::underscore_cursor);
  }

  if ( linux_terminal && getFramebuffer_bpp() >= 4 )
    max_color = 16;

  if ( kde_konsole )
    setKDECursor(fc::UnderlineCursor);

  if (  max_color >= 16
     && ! cygwin_terminal
     && ! kde_konsole
     && ! tera_terminal )
  {
    resetColorMap();
    saveColorMap();

    setPalette (fc::Black, 0x00, 0x00, 0x00);
    setPalette (fc::Blue, 0x22, 0x22, 0xb2);
    setPalette (fc::Green, 0x18, 0xb2, 0x18);
    setPalette (fc::Cyan, 0x4a, 0x4a, 0xe4);
    setPalette (fc::Red, 0xb2, 0x18, 0x18);
    setPalette (fc::Magenta, 0xb2, 0x18, 0xb2);
    setPalette (fc::Brown, 0xe8, 0x87, 0x1f);
    setPalette (fc::LightGray, 0xbc, 0xbc, 0xbc);
    setPalette (fc::DarkGray, 0x50, 0x50, 0x50);
    setPalette (fc::LightBlue, 0x80, 0xa4, 0xec);
    setPalette (fc::LightGreen, 0x54, 0xff, 0x54);
    setPalette (fc::LightCyan, 0x49, 0xc9, 0xe3);
    setPalette (fc::LightRed, 0xff, 0x54, 0x54);
    setPalette (fc::LightMagenta, 0xff, 0x54, 0xff);
    setPalette (fc::Yellow, 0xff, 0xff, 0x54);
    setPalette (fc::White, 0xff, 0xff, 0xff);
  }

  // set 200 Hz beep (100 ms)
  setBeep(200, 100);

  signal(SIGTERM,  FTerm::signal_handler); // Termination signal
  signal(SIGQUIT,  FTerm::signal_handler); // Quit from keyboard (Ctrl-\)
  signal(SIGINT,   FTerm::signal_handler); // Keyboard interrupt (Ctrl-C)
  signal(SIGABRT,  FTerm::signal_handler); // Abort signal from abort(3)
  signal(SIGILL,   FTerm::signal_handler); // Illegal Instruction
  signal(SIGSEGV,  FTerm::signal_handler); // Invalid memory reference
  signal(SIGWINCH, FTerm::signal_handler); // Window resize signal
}

//----------------------------------------------------------------------
void FTerm::finish()
{
  // set default signal handler
  signal(SIGWINCH, SIG_DFL); // Window resize signal
  signal(SIGSEGV,  SIG_DFL); // Invalid memory reference
  signal(SIGILL,   SIG_DFL); // Illegal Instruction
  signal(SIGABRT,  SIG_DFL); // Abort signal from abort(3)
  signal(SIGINT,   SIG_DFL); // Keyboard interrupt (Ctrl-C)
  signal(SIGQUIT,  SIG_DFL); // Quit from keyboard (Ctrl-\)
  signal(SIGTERM,  SIG_DFL); // Termination signal

  if ( xterm_title && xterm && ! rxvt_terminal )
    setXTermTitle (*xterm_title);

  showCursor();
  setCookedMode(); // leave raw mode

  // turn off all attributes
  if ( tcap[t_exit_attribute_mode].string )
  {
    putstring (tcap[t_exit_attribute_mode].string);
    fflush(stdout);
  }

  // turn off pc charset mode
  if ( tcap[t_exit_pc_charset_mode].string )
  {
    putstring (tcap[t_exit_pc_charset_mode].string);
    fflush(stdout);
  }

  // reset xterm color settings to default
  setXTermCursorColor("rgb:b1b1/b1b1/b1b1");
  resetXTermMouseForeground();
  resetXTermMouseBackground();
  resetXTermCursorColor();
  resetXTermForeground();
  resetXTermBackground();
  resetXTermHighlightBackground();
  setXTermCursorStyle(fc::steady_block);

  if ( max_color >= 16 && ! kde_konsole && ! tera_terminal )
  {
    // reset screen settings
    setPalette (fc::Cyan, 0x18, 0xb2, 0xb2);
    setPalette (fc::LightGray, 0xb2, 0xb2, 0xb2);
    setPalette (fc::DarkGray, 0x68, 0x68, 0x68);
    setPalette (fc::LightBlue, 0x54, 0x54, 0xff);
    setPalette (fc::LightGreen, 0x54, 0xff, 0x54);

    resetXTermColors();
    resetColorMap();
  }

  if ( mintty_terminal )
  {
    //  normal escape key mode
    putstring (CSI "?7727l");
    fflush(stdout);
  }

  if ( linux_terminal )
  {
    setLightBackgroundColors (false);
    setConsoleCursor(fc::default_cursor);
  }

  if ( kde_konsole )
    setKDECursor(fc::BlockCursor);

  resetBeep();

  if (  linux_terminal
     && background_color_erase
     && tcap[t_clear_screen].string )
  {
    int rows = term->getHeight();
    putstring (tcap[t_clear_screen].string, rows);
  }

  if ( mouse_support )
    disableXTermMouse();

#ifdef F_HAVE_LIBGPM
  if ( gpm_mouse_enabled )
    disableGpmMouse();  // Disable gpm server
#endif

  // restores the screen and the cursor position
  if ( tcap[t_exit_ca_mode].string )
  {
    putstring (tcap[t_exit_ca_mode].string);
    fflush(stdout);
  }

  // restore cursor to position of last save_cursor
  if ( tcap[t_restore_cursor].string )
  {
    putstring (tcap[t_restore_cursor].string);
    fflush(stdout);
  }

  // leave 'keyboard_transmit' mode
  if ( tcap[t_keypad_local].string )
  {
    putstring (tcap[t_keypad_local].string);
    fflush(stdout);
  }

  if ( linux_terminal && utf8_console )
    setUTF8(true);

  if ( NewFont || VGAFont )
    setOldFont();
  else
  {
    if ( screenFont.data != 0 )
      delete[] screenFont.data;
    if ( screenUnicodeMap.entries != 0 )
      delete[] screenUnicodeMap.entries;
  }

  if ( encoding_set )
    delete encoding_set;
  if ( vt100_alt_char )
    delete vt100_alt_char;
  if ( output_buffer )
    delete output_buffer;
  if ( Sec_DA )
    delete Sec_DA;
  if ( AnswerBack )
    delete AnswerBack;
  if ( xterm_title )
    delete xterm_title;
  if ( xterm_font )
    delete xterm_font;

  if ( vdesktop != 0 )
  {
    if ( vdesktop->changes != 0 )
      delete[] vdesktop->changes;
    if ( vdesktop->text != 0 )
      delete[] vdesktop->text;
    delete vdesktop;
  }
  if ( vterm != 0 )
  {
    if ( vterm->changes != 0 )
    {
      delete[] vterm->changes;
      vterm->changes = 0;
    }
    if ( vterm->text != 0 )
    {
      delete[] vterm->text;
      vterm->text = 0;
    }
    delete vterm;
  }
}

//----------------------------------------------------------------------
inline uInt FTerm::charEncode (uInt c)
{
  return charEncode (c, Encoding);
}

//----------------------------------------------------------------------
uInt FTerm::charEncode (uInt c, fc::encoding enc)
{
  for (uInt i=0; i <= uInt(lastCharItem); i++)
  {
    if ( character[i][0] == c )
    {
      c = character[i][enc];
      break;
    }
  }
  return c;
}

//----------------------------------------------------------------------
uInt FTerm::cp437_to_unicode (uChar c)
{
  register uInt ucs = uInt(c);

  for (register uInt i=0; i <= lastCP437Item; i++)
  {
    if ( cp437_to_ucs[i][0] == c ) // found
    {
      ucs = cp437_to_ucs[i][1];
      break;
    }
  }
  return ucs;
}

// protected methods of FTerm
//----------------------------------------------------------------------
bool FTerm::charEncodable (uInt c)
{
  uInt ch = charEncode(c);

  return bool(ch > 0 && ch != c);
}

//----------------------------------------------------------------------
void FTerm::createArea (term_area*& area)
{
  // initialize virtual window
  area = new term_area;

  area->width         = -1;
  area->height        = -1;
  area->right_shadow  = 0;
  area->bottom_shadow = 0;
  area->changes       = 0;
  area->text          = 0;
  area->visible       = false;
  area->widget        = static_cast<FWidget*>(this);

  resizeArea (area);
}

//----------------------------------------------------------------------
void FTerm::resizeArea (term_area* area)
{
  int area_size, width, height, rsw, bsh;
  FOptiAttr::char_data default_char;
  line_changes unchanged;

  if ( ! area )
    return;

  if ( term_object == this )
  {
    rsw = bsh = 0;  // no shadow
    width = getColumnNumber();
    height = getLineNumber();
    area_size = width * height;
  }
  else
  {
    FWidget* w = static_cast<FWidget*>(this);
    rsw = w->getShadow().getX();  // right shadow width;
    bsh = w->getShadow().getY();  // bottom shadow height
    width = w->getWidth();
    height = w->getHeight();
    area_size = (width+rsw) * (height+bsh);
  }

  if ( area->height + area->bottom_shadow != height + bsh )
  {
    if ( area->changes != 0 )
      delete[] area->changes;
    if ( area->text != 0 )
      delete[] area->text;
    area->changes = new line_changes[height + bsh];
    area->text    = new FOptiAttr::char_data[area_size];
  }
  else if ( area->width + area->right_shadow != width + rsw )
  {
    if ( area->text != 0 )
      delete[] area->text;
    area->text = new FOptiAttr::char_data[area_size];
  }
  else
    return;

  area->width = width;
  area->height = height;
  area->right_shadow = rsw;
  area->bottom_shadow = bsh;

  default_char.code          = ' ';
  default_char.fg_color      = fc::Default;
  default_char.bg_color      = fc::Default;
  default_char.bold          = 0;
  default_char.dim           = 0;
  default_char.italic        = 0;
  default_char.underline     = 0;
  default_char.blink         = 0;
  default_char.reverse       = 0;
  default_char.standout      = 0;
  default_char.invisible     = 0;
  default_char.protect       = 0;
  default_char.crossed_out   = 0;
  default_char.dbl_underline = 0;
  default_char.alt_charset   = 0;
  default_char.pc_charset    = 0;

  std::fill_n (area->text, area_size, default_char);

  unchanged.xmin = uInt(width+rsw);
  unchanged.xmax = 0;

  std::fill_n (area->changes, height+bsh, unchanged);
}

//----------------------------------------------------------------------
void FTerm::restoreVTerm (const FRect& box)
{
  restoreVTerm ( box.getX()
               , box.getY()
               , box.getWidth()
               , box.getHeight() );
}

//----------------------------------------------------------------------
void FTerm::restoreVTerm (int x, int y, int w, int h)
{
  FOptiAttr::char_data* tc; // terminal character
  FOptiAttr::char_data* sc; // shown character
  FWidget* widget;

  x--;
  y--;
  if ( x < 0 )
    x = 0;
  if ( y < 0 )
    y = 0;
  if ( w < 0 || h < 0 )
    return;

  if ( x+w > vterm->width )
    w = vterm->width - x;
  if ( w < 0 )
    return;
  if ( y+h > vterm->height )
    h = vterm->height - y;
  if ( h < 0 )
    return;

  widget = static_cast<FWidget*>(this);

  for (register int ty=0; ty < h; ty++)
  {
    for (register int tx=0; tx < w; tx++)
    {
      tc = &vterm->text[(y+ty) * vterm->width + (x+tx)];
      sc = &vdesktop->text[(y+ty) * vdesktop->width + (x+tx)];

      if ( widget->window_list && ! widget->window_list->empty() )
      {
        FWidget::widgetList::const_iterator iter, end;
        iter = widget->window_list->begin();
        end  = widget->window_list->end();

        while ( iter != end )
        {
          const FRect& geometry = (*iter)->getGeometryGlobalShadow();
          if ( geometry.contains(x+tx+1, y+ty+1) )
          {
            int win_x = (*iter)->getGlobalX() - 1;
            int win_y = (*iter)->getGlobalY() - 1;
            term_area* win = (*iter)->getVWin();
            int line_len = win->width + win->right_shadow;
            if ( win->visible )
              sc = &win->text[(y+ty-win_y) * line_len + (x+tx-win_x)];
          }
          ++iter;
        }
      }

      // menubar is always on top
      FWidget* menubar;
      menubar = reinterpret_cast<FWidget*>(FWidget::menuBar());

      if (  vmenubar && menubar
         && menubar->getGeometryGlobal().contains(x+tx+1, y+ty+1) )
      {
        int bar_x = menubar->getGlobalX() - 1;
        int bar_y = menubar->getGlobalY() - 1;
        if ( vmenubar->visible )
          sc = &vmenubar->text[(y+ty-bar_y) * vmenubar->width + (x+tx-bar_x)];
      }

      // statusbar is always on top
      FWidget* statusbar;
      statusbar = reinterpret_cast<FWidget*>(FWidget::statusBar());

      if (  vstatusbar && statusbar
         && statusbar->getGeometryGlobal().contains(x+tx+1, y+ty+1) )
      {
        int bar_x = statusbar->getGlobalX() - 1;
        int bar_y = statusbar->getGlobalY() - 1;
        if ( vstatusbar->visible )
          sc = &vstatusbar->text[(y+ty-bar_y) * vstatusbar->width + (x+tx-bar_x)];
      }

      memcpy (tc, sc, sizeof(FOptiAttr::char_data));

      if ( short(vterm->changes[y+ty].xmin) > x )
        vterm->changes[y+ty].xmin = uInt(x);
      if ( short(vterm->changes[y+ty].xmax) < x+w-1 )
        vterm->changes[y+ty].xmax = uInt(x+w-1);
    }
  }
}

//----------------------------------------------------------------------
bool FTerm::isCovered(int x, int y, FTerm::term_area* area) const
{
  bool covered, found;
  FWidget* w;

  if ( ! area )
    return false;

  covered = false;
  found = bool(area == vdesktop);
  x++;
  y++;

  w = static_cast<FWidget*>(area->widget);

  if ( w->window_list && ! w->window_list->empty() )
  {
    FWidget::widgetList::const_iterator iter, end;
    iter = w->window_list->begin();
    end  = w->window_list->end();

    while ( iter != end )
    {
      const FRect& geometry = (*iter)->getGeometryGlobalShadow();
      if (  found
         && (*iter)->isVisible()
         && (*iter)->isShown()
         && geometry.contains(x,y) )
      {
        covered = true;
        break;
      }
      if ( area == (*iter)->getVWin() )
        found = true;
      ++iter;
    }
  }

  // menubar is always on top
  FWidget* menubar;
  if ( vmenubar )
    menubar = reinterpret_cast<FWidget*>(vmenubar->widget);
  else
    menubar = 0;

  if (  area != vmenubar && menubar
     && menubar->getGeometryGlobal().contains(x,y) )
  {
    covered = true;
  }

  // statusbar is always on top
  FWidget* statusbar;
  if ( vstatusbar )
    statusbar = reinterpret_cast<FWidget*>(vstatusbar->widget);
  else
    statusbar = 0;

  if (  area != vstatusbar && statusbar
     && statusbar->getGeometryGlobal().contains(x,y) )
  {
    covered = true;
  }

  return covered;
}

//----------------------------------------------------------------------
void FTerm::updateVTerm (FTerm::term_area* area)
{
  int ax, ay, aw, ah, rsh, bsh, y_end, ol;
  FOptiAttr::char_data* tc; // terminal character
  FOptiAttr::char_data* ac; // area character

  if ( ! vterm_updates )
  {
    last_area = area;
    return;
  }

  if ( ! area )
    return;

  if ( ! area->visible )
    return;

  ax  = area->widget->getGlobalX() - 1;
  ay  = area->widget->getGlobalY() - 1;
  aw  = area->width;
  ah  = area->height;
  rsh = area->right_shadow;
  bsh = area->bottom_shadow;
  ol  = 0;  // outside left

  if ( ax < 0 )
  {
    ol = abs(ax);
    ax = 0;
  }
  if ( ah + bsh + ay > vterm->height )
    y_end = vterm->height - ay;
  else
    y_end = ah + bsh;

  for (register int y=0; y < y_end; y++)
  {
    int line_xmin = int(area->changes[y].xmin);
    int line_xmax = int(area->changes[y].xmax);
    if ( line_xmin <= line_xmax )
    {
      int _xmin, _xmax;

      if ( ax == 0 )
        line_xmin = ol;
      if ( aw + rsh + ax - ol >= vterm->width )
        line_xmax = vterm->width + ol - ax - 1;

      if ( ax + line_xmin >= vterm->width )
        continue;

      for (register int x=line_xmin; x <= line_xmax; x++)
      {
        int gx, gy, line_len;

        gx = ax + x;  // global position
        gy = ay + y;
        if ( gx < 0 || gy < 0 )
          continue;
        line_len = aw + rsh;

        ac = &area->text[y * line_len + x];
        tc = &vterm->text[gy * vterm->width + gx - ol];

        if ( ! isCovered(gx-ol, gy, area) )
          memcpy (tc, ac, sizeof(FOptiAttr::char_data));
        else
          line_xmin++;  // don't update covered character
      }
      _xmin = ax + line_xmin - ol;
      _xmax = ax + line_xmax;
      if ( _xmin < short(vterm->changes[ay+y].xmin) )
        vterm->changes[ay+y].xmin = uInt(_xmin);
      if ( _xmax > short(vterm->changes[ay+y].xmax) )
        vterm->changes[ay+y].xmax = uInt(_xmax);
      area->changes[y].xmin = uInt(aw + rsh);
      area->changes[y].xmax = 0;
    }
  }
}

//----------------------------------------------------------------------
void FTerm::setUpdateVTerm (bool on)
{
  vterm_updates = on;
  if ( on )
    updateVTerm (last_area);
}

//----------------------------------------------------------------------
void FTerm::getArea (int ax, int ay, FTerm::term_area* area)
{
  int y_end;
  int length;
  FOptiAttr::char_data* tc; // terminal character
  FOptiAttr::char_data* ac; // area character

  if ( ! area )
    return;
  ax--;
  ay--;

  if ( area->height+ay > vterm->height )
    y_end = area->height - ay;
  else
    y_end = area->height;

  if ( area->width+ax > vterm->width )
    length = vterm->width - ax;
  else
    length = area->width;

  for (int y=0; y < y_end; y++)
  {
    ac = &area->text[y * area->width];
    tc = &vterm->text[(ay+y) * vterm->width + ax];
    memcpy (ac, tc, sizeof(FOptiAttr::char_data) * unsigned(length));

    if ( short(area->changes[y].xmin) > 0 )
      area->changes[y].xmin = 0;
    if ( short(area->changes[y].xmax) < length-1 )
      area->changes[y].xmax = uInt(length-1);
  }
}

//----------------------------------------------------------------------
void FTerm::getArea (int x, int y, int w, int h, FTerm::term_area* area)
{
  int y_end, length, dx, dy;
  FOptiAttr::char_data* tc; // terminal character
  FOptiAttr::char_data* ac; // area character

  if ( ! area )
    return;

  dx = x - area->widget->getGlobalX();
  dy = y - area->widget->getGlobalY();

  if ( x < 0 || y < 0 )
    return;

  if ( y-1+h > vterm->height )
    y_end = vterm->height - y + 1;
  else
    y_end = h - 1;

  if ( x-1+w > vterm->width )
    length = vterm->width - x + 1;
  else
    length = w;
  if ( length < 1 )
    return;

  for (int _y=0; _y < y_end; _y++)
  {
    int line_len = area->width + area->right_shadow;
    tc = &vterm->text[(y+_y-1) * vterm->width + x-1];
    ac = &area->text[(dy+_y) * line_len + dx];

    memcpy (ac, tc, sizeof(FOptiAttr::char_data) * unsigned(length));

    if ( short(area->changes[dy+_y].xmin) > dx )
      area->changes[dy+_y].xmin = uInt(dx);
    if ( short(area->changes[dy+_y].xmax) < dx+length-1 )
      area->changes[dy+_y].xmax = uInt(dx+length-1);
  }
}

//----------------------------------------------------------------------
void FTerm::putArea (const FPoint& pos, FTerm::term_area* area)
{
  if ( ! area )
    return;
  if ( ! area->visible )
    return;
  putArea (pos.getX(), pos.getY(), area);
}

//----------------------------------------------------------------------
void FTerm::putArea (int ax, int ay, FTerm::term_area* area)
{
  int aw, ah, rsh, bsh, y_end, length, ol, sbar;
  FOptiAttr::char_data* tc; // terminal character
  FOptiAttr::char_data* ac; // area character

  if ( ! area )
    return;
  if ( ! area->visible )
    return;

  ax--;
  ay--;
  aw   = area->width;
  ah   = area->height;
  rsh  = area->right_shadow;
  bsh  = area->bottom_shadow;
  ol   = 0;  // outside left
  sbar = 0;  // statusbar distance

  if ( ax < 0 )
  {
    ol = abs(ax);
    ax = 0;
  }

  if ( vstatusbar && vstatusbar->widget && area != vstatusbar )
    sbar = 1;

  if ( ay + ah + bsh + sbar > vterm->height )
    y_end = vterm->height - ay - sbar;
  else
    y_end = ah + bsh;

  if ( aw + rsh - ol + ax > vterm->width )
    length = vterm->width - ax;
  else
    length = aw + rsh - ol;

  if ( length < 1 )
    return;

  for (int y=0; y < y_end; y++)
  {
    int line_len = aw + rsh;

    tc = &vterm->text[(ay+y) * vterm->width + ax];
    ac = &area->text[y * line_len + ol];

    memcpy (tc, ac, sizeof(FOptiAttr::char_data) * unsigned(length));

    if ( ax < short(vterm->changes[ay+y].xmin) )
      vterm->changes[ay+y].xmin = uInt(ax);
    if ( ax+length-1 > short(vterm->changes[ay+y].xmax) )
      vterm->changes[ay+y].xmax = uInt(ax+length-1);
  }
}

//----------------------------------------------------------------------
FOptiAttr::char_data FTerm::getCoveredCharacter (int x, int y, FTerm* obj)
{
  int xx,yy;
  FOptiAttr::char_data* cc; // covered character
  FWidget* w;

  x--;
  y--;
  xx = x;
  yy = y;

  if ( xx < 0 )
    xx = 0;
  if ( yy < 0 )
    yy = 0;

  if ( xx >= vterm->width )
    xx = vterm->width - 1;
  if ( yy >= vterm->height )
    yy = vterm->height - 1;

  cc = &vdesktop->text[yy * vdesktop->width + xx];
  w = static_cast<FWidget*>(obj);

  if ( w->window_list && ! w->window_list->empty() )
  {
    FWidget::widgetList::const_iterator iter, end;
    int layer = FWindow::getWindowLayer(w);

    iter = w->window_list->begin();
    end  = w->window_list->end();

    while ( iter != end )
    {
      if ( obj && *iter != obj && layer >= FWindow::getWindowLayer(*iter) )
      {
        const FRect& geometry = (*iter)->getGeometryGlobalShadow();
        if ( geometry.contains(x+1,y+1) )
        {
          int win_x = (*iter)->getGlobalX() - 1;
          int win_y = (*iter)->getGlobalY() - 1;
          term_area* win = (*iter)->getVWin();
          int line_len = win->width + win->right_shadow;
          if ( win->visible )
            cc = &win->text[(y-win_y) * line_len + (x-win_x)];
        }
      }
      else
        break;
      ++iter;
    }
  }

  return *cc;
}


// public methods of FTerm
//----------------------------------------------------------------------
bool FTerm::setVGAFont()
{
  if ( VGAFont )
    return VGAFont;

  VGAFont = true;

  if ( xterm || screen_terminal || osc_support )
  {
    // Set font in xterm to vga
    oscPrefix();
    putstring (OSC "50;vga" BEL);
    oscPostfix();
    fflush(stdout);
    NewFont = false;
    pc_charset_console = true;
    Encoding = fc::PC;
    if ( xterm && utf8_console )
      Fputchar = &FTerm::putchar_UTF8;
    else
      Fputchar = &FTerm::putchar_ASCII;
  }
  else if ( linux_terminal )
  {
    if ( openConsole() == 0 )
    {
      if ( isConsole() )
      {
        // standard vga font 8x16
        int ret = setScreenFont(__8x16std, 256, 8, 16);
        if ( ret != 0 )
          VGAFont = false;
        // unicode character mapping
        struct unimapdesc unimap;
        unimap.entry_ct = uChar ( sizeof(unicode_cp437_pairs)
                                / sizeof(unipair) );
        unimap.entries = &unicode_cp437_pairs[0];
        setUnicodeMap(&unimap);
      }
      else
        VGAFont = false;
      getTermSize();
      closeConsole();
    }
    else
      VGAFont = false;

    pc_charset_console = true;
    Encoding = fc::PC;
    Fputchar = &FTerm::putchar_ASCII;
  }
  else
    VGAFont = false;

  return VGAFont;
}

//----------------------------------------------------------------------
bool FTerm::setNewFont()
{
  if ( NewFont )
    return true;

  if ( xterm || screen_terminal || urxvt_terminal || osc_support )
  {
    NewFont = true;
    // Set font in xterm to 8x16graph
    oscPrefix();
    putstring (OSC "50;8x16graph" BEL);
    oscPostfix();
    fflush(stdout);
    pc_charset_console = true;
    Encoding = fc::PC;
    if ( xterm && utf8_console )
      Fputchar = &FTerm::putchar_UTF8;
    else
      Fputchar = &FTerm::putchar_ASCII;
  }
  else if ( linux_terminal )
  {
    NewFont = true;

    if ( openConsole() == 0 )
    {
      if ( isConsole() )
      {
        struct unimapdesc unimap;
        int ret;

        // Set the graphical font 8x16
        ret = setScreenFont(__8x16graph, 256, 8, 16);
        if ( ret != 0 )
          NewFont = false;
        // unicode character mapping
        unimap.entry_ct = uInt16 ( sizeof(unicode_cp437_pairs)
                                 / sizeof(unipair) );
        unimap.entries = &unicode_cp437_pairs[0];
        setUnicodeMap(&unimap);
      }
      getTermSize();
      closeConsole();
    }
    pc_charset_console = true;
    Encoding = fc::PC;
    Fputchar = &FTerm::putchar_ASCII;  // function pointer
  }
  else
    NewFont = false;

  return NewFont;
}

//----------------------------------------------------------------------
bool FTerm::setOldFont()
{
  bool retval;

  if ( ! NewFont && ! VGAFont )
    return false;

  retval  = \
  NewFont = \
  VGAFont = false;

  if ( xterm || screen_terminal || urxvt_terminal || osc_support )
  {
    if ( xterm_font && xterm_font->getLength() > 2 )
    {
      // restore saved xterm font
      oscPrefix();
      putstringf (OSC "50;%s" BEL, xterm_font->c_str() );
      oscPostfix();
    }
    else
    {
      // Set font in xterm to vga
      oscPrefix();
      putstring (OSC "50;vga" BEL);
      oscPostfix();
    }
    fflush(stdout);
    retval = true;
  }
  else if ( linux_terminal )
  {
    if ( openConsole() == 0 )
    {
      if ( isConsole() )
      {
        if ( screenFont.data )
        {
          int ret = setScreenFont ( screenFont.data
                                  , screenFont.charcount
                                  , screenFont.width
                                  , screenFont.height
                                  , true );
          delete[] screenFont.data;
          if ( ret == 0 )
            retval = true;
        }
        if ( screenUnicodeMap.entries )
        {
          setUnicodeMap (&screenUnicodeMap);
          delete[] screenUnicodeMap.entries;
        }
      }
      getTermSize();
      closeConsole();
    }
  }
  return retval;
}

//----------------------------------------------------------------------
void FTerm::setConsoleCursor (fc::console_cursor_style style)
{
  // Set cursor style in linux console
  if ( linux_terminal )
  {
    consoleCursorStyle = style;
    if ( hiddenCursor )
      return;
    putstringf (CSI "?%dc", style);
    fflush(stdout);
  }
}

//----------------------------------------------------------------------
void FTerm::getTermSize()
{
  struct winsize win_size;
  int ret;

  if ( fd_tty < 0 )
    return;

  ret = ioctl (fd_tty, TIOCGWINSZ, &win_size);

  if ( ret != 0 || win_size.ws_col == 0 || win_size.ws_row == 0 )
  {
    char* str;
    term->setX(1);
    term->setY(1);
    str = getenv("COLUMNS");
    term->setWidth(str ? atoi(str) : 80);
    str = getenv("LINES");
    term->setHeight(str ? atoi(str) : 25);
  }
  else
  {
    term->setRect(1, 1, win_size.ws_col, win_size.ws_row);
  }

  opti_move->setTermSize (term->getWidth(), term->getHeight());
}

//----------------------------------------------------------------------
void FTerm::setTermSize (int term_width, int term_height)
{
  // Set xterm size to {term_width} x {term_height}
  if ( xterm )
  {
    putstringf (CSI "8;%d;%dt", term_height, term_width);
    fflush(stdout);
  }
}

//----------------------------------------------------------------------
void FTerm::createVTerm()
{
  // initialize virtual terminal
  vterm = new term_area;
  vterm->width         = -1;
  vterm->height        = -1;
  vterm->right_shadow  = 0;
  vterm->bottom_shadow = 0;
  vterm->changes       = 0;
  vterm->text          = 0;
  vterm->visible       = true;
  vterm->widget        = static_cast<FWidget*>(this);

  resizeVTerm();
}

//----------------------------------------------------------------------
void FTerm::resizeVTerm()
{
  FOptiAttr::char_data default_char;
  line_changes unchanged;
  int term_width, term_height, vterm_size;

  term_width  = term->getWidth();
  term_height = term->getHeight();
  vterm_size  = term_width * term_height;

  if ( vterm->height != term_height )
  {
    if ( vterm->changes != 0 )
    {
      delete[] vterm->changes;
      vterm->changes = 0;
    }
    if ( vterm->text != 0 )
    {
      delete[] vterm->text;
      vterm->text = 0;
    }

    vterm->changes = new line_changes[term_height];
    vterm->text    = new FOptiAttr::char_data[vterm_size];
  }
  else if ( vterm->width != term_width )
  {
    if ( vterm->text != 0 )
    {
      delete[] vterm->text;
      vterm->text = 0;
    }
    vterm->text = new FOptiAttr::char_data[vterm_size];
  }
  else
    return;

  vterm->width  = term_width;
  vterm->height = term_height;

  default_char.code          = ' ';
  default_char.fg_color      = fc::Default;
  default_char.bg_color      = fc::Default;
  default_char.bold          = 0;
  default_char.dim           = 0;
  default_char.italic        = 0;
  default_char.underline     = 0;
  default_char.blink         = 0;
  default_char.reverse       = 0;
  default_char.standout      = 0;
  default_char.invisible     = 0;
  default_char.protect       = 0;
  default_char.crossed_out   = 0;
  default_char.dbl_underline = 0;
  default_char.alt_charset   = 0;
  default_char.pc_charset    = 0;

  std::fill_n (vterm->text, vterm_size, default_char);

  unchanged.xmin = uInt(term_width);
  unchanged.xmax = 0;

  std::fill_n (vterm->changes, term_height, unchanged);
}

//----------------------------------------------------------------------
void FTerm::putVTerm()
{
  for (int i=0; i < vterm->height; i++)
  {
    vterm->changes[i].xmin = 0;
    vterm->changes[i].xmax = uInt(vterm->width - 1);
  }
  updateTerminal();
}

//----------------------------------------------------------------------
void FTerm::updateTerminal()
{
  term_area* vt;
  FWidget* focus_widget;
  FApplication* fapp;
  int term_width, term_height;

  if ( static_cast<FApplication*>(term_object)->isQuit() )
    return;

  if ( ! force_terminal_update )
  {
    if ( ! terminal_updates )
      return;

    if ( input_data_pending )
    {
      terminal_update_pending = true;
      return;
    }
  }

  vt = vterm;
  term_width = term->getWidth() - 1;
  term_height = term->getHeight() - 1;

  for (register uInt y=0; y < uInt(vt->height); y++)
  {
    uInt change_xmax = vt->changes[y].xmax;
    if ( vt->changes[y].xmin <= vt->changes[y].xmax )
    {
      uInt change_xmin = vt->changes[y].xmin;
      setTermXY (int(change_xmin), int(y));

      for ( register uInt x=change_xmin;
            x <= change_xmax;
            x++ )
      {
        FOptiAttr::char_data* print_char;
        print_char = &vt->text[y * uInt(vt->width) + x];

        if (  x_term_pos == term_width
           && y_term_pos == term_height )
          appendLowerRight (print_char);
        else
          appendCharacter (print_char);

        x_term_pos++;
      }
      vt->changes[y].xmin = uInt(vt->width);
      vt->changes[y].xmax = 0;
    }
    // cursor wrap
    if ( x_term_pos > term_width )
    {
      if ( y_term_pos == term_height )
        x_term_pos--;
      else
      {
        if ( eat_nl_glitch )
        {
          x_term_pos = -1;
          y_term_pos = -1;
        }
        else if ( automatic_right_margin )
        {
          x_term_pos = 0;
          y_term_pos++;
        }
        else
          x_term_pos--;
      }
    }
  }

  // set the cursor to the focus widget
  fapp = static_cast<FApplication*>(term_object);
  focus_widget = fapp->focusWidget();

  if (  focus_widget
     && focus_widget->isVisible()
     && focus_widget->isCursorInside() )
  {
    focus_widget->setCursor();
    if ( focus_widget->hasVisibleCursor() )
      showCursor();
  }
}

//----------------------------------------------------------------------
void FTerm::setKDECursor (fc::kde_konsole_CursorShape style)
{
  // Set cursor style in KDE konsole
  if ( kde_konsole )
  {
    oscPrefix();
    putstringf (OSC "50;CursorShape=%d" BEL, style);
    oscPostfix();
    fflush(stdout);
  }
}

//----------------------------------------------------------------------
FString FTerm::getXTermFont()
{
  FString font("");
  if ( xterm || screen_terminal || osc_support )
  {
    if ( raw_mode && non_blocking_stdin )
    {
      int n;
      char temp[150] = {};
      oscPrefix();
      putstring (OSC "50;?" BEL);  // get font
      oscPostfix();
      fflush(stdout);
      usleep(150000);  // wait 150 ms

      // read the terminal answer
      n = int(read(fileno(stdin), &temp, sizeof(temp)-1));

      // BEL + '\0' = string terminator
      if ( n >= 6 && temp[n-1] == BEL[0] && temp[n] == '\0' )
      {
        temp[n-1] = '\0';
        font = static_cast<char*>(temp + 5);
      }
    }
  }
  return font;
}

//----------------------------------------------------------------------
FString FTerm::getXTermTitle()
{
  FString title("");

  if ( kde_konsole )
    return title;

  if ( raw_mode && non_blocking_stdin )
  {
    int n;
    char temp[512] = {};
    putstring (CSI "21t");  // get title
    fflush(stdout);
    usleep(150000);  // wait 150 ms

    // read the terminal answer
    n = int(read(fileno(stdin), &temp, sizeof(temp)-1));

    // Esc + \ = OSC string terminator
    if ( n >= 5 && temp[n-1] == '\\' && temp[n-2] == ESC[0] )
    {
      temp[n-2] = '\0';
      title = static_cast<char*>(temp + 3);
    }
  }
  return title;
}

//----------------------------------------------------------------------
void FTerm::setXTermCursorStyle (fc::xterm_cursor_style style)
{
  // Set the xterm cursor style
  if ( (xterm || mintty_terminal) && ! gnome_terminal && ! kde_konsole )
  {
    putstringf (CSI "%d q", style);
    fflush(stdout);
  }
}

//----------------------------------------------------------------------
void FTerm::setXTermTitle (const FString& title)
{
  // Set the xterm title
  if ( xterm || screen_terminal
     || mintty_terminal || putty_terminal
     || osc_support )
  {
    oscPrefix();
    putstringf (OSC "0;%s" BEL, title.c_str());
    oscPostfix();
    fflush(stdout);
  }
}

//----------------------------------------------------------------------
void FTerm::setXTermForeground (const FString& fg)
{
  // Set the VT100 text foreground color
  if ( xterm || screen_terminal
     || mintty_terminal || mlterm_terminal
     || osc_support )
  {
    oscPrefix();
    putstringf (OSC "10;%s" BEL, fg.c_str());
    oscPostfix();
    fflush(stdout);
  }
}

//----------------------------------------------------------------------
void FTerm::setXTermBackground (const FString& bg)
{
  // Set the VT100 text background color
  if ( xterm || screen_terminal
     || mintty_terminal || mlterm_terminal
     || osc_support )
  {
    oscPrefix();
    putstringf (OSC "11;%s" BEL, bg.c_str());
    oscPostfix();
    fflush(stdout);
  }
}

//----------------------------------------------------------------------
void FTerm::setXTermCursorColor (const FString& cc)
{
  // Set the text cursor color
  if ( xterm || screen_terminal
     || mintty_terminal || urxvt_terminal
     || osc_support )
  {
    oscPrefix();
    putstringf (OSC "12;%s" BEL, cc.c_str());
    oscPostfix();
    fflush(stdout);
  }
}

//----------------------------------------------------------------------
void FTerm::setXTermMouseForeground (const FString& mfg)
{
  // Set the mouse foreground color
  if ( xterm || screen_terminal || urxvt_terminal || osc_support )
  {
    oscPrefix();
    putstringf (OSC "13;%s" BEL, mfg.c_str());
    oscPostfix();
    fflush(stdout);
  }
}

//----------------------------------------------------------------------
void FTerm::setXTermMouseBackground (const FString& mbg)
{
  // Set the mouse background color
  if ( xterm || screen_terminal || osc_support )
  {
    oscPrefix();
    putstringf (OSC "14;%s" BEL, mbg.c_str());
    oscPostfix();
    fflush(stdout);
  }
}

//----------------------------------------------------------------------
void FTerm::setXTermHighlightBackground (const FString& hbg)
{
  // Set the highlight background color
  if ( xterm || screen_terminal || urxvt_terminal || osc_support )
  {
    oscPrefix();
    putstringf (OSC "17;%s" BEL, hbg.c_str());
    oscPostfix();
    fflush(stdout);
  }
}

//----------------------------------------------------------------------
void FTerm::resetXTermColors()
{
  // Reset the entire color table
  if ( xterm || screen_terminal || osc_support )
  {
    oscPrefix();
    putstringf (OSC "104" BEL);
    oscPostfix();
    fflush(stdout);
  }
}

//----------------------------------------------------------------------
void FTerm::resetXTermForeground()
{
  // Reset the VT100 text foreground color
  if ( xterm || screen_terminal || osc_support )
  {
    oscPrefix();
    putstring (OSC "110" BEL);
    oscPostfix();
    fflush(stdout);
  }
}

//----------------------------------------------------------------------
void FTerm::resetXTermBackground()
{
  // Reset the VT100 text background color
  if ( xterm || screen_terminal || osc_support )
  {
    oscPrefix();
    putstring (OSC "111" BEL);
    oscPostfix();
    fflush(stdout);
  }
}

//----------------------------------------------------------------------
void FTerm::resetXTermCursorColor()
{
  // Reset the text cursor color
  if ( xterm || screen_terminal || osc_support )
  {
    oscPrefix();
    putstring (OSC "112" BEL);
    oscPostfix();
    fflush(stdout);
  }
}

//----------------------------------------------------------------------
void FTerm::resetXTermMouseForeground()
{
  // Reset the mouse foreground color
  if ( xterm || screen_terminal || osc_support )
  {
    oscPrefix();
    putstring (OSC "113" BEL);
    oscPostfix();
    fflush(stdout);
  }
}

//----------------------------------------------------------------------
void FTerm::resetXTermMouseBackground()
{
  // Reset the mouse background color
  if ( xterm || screen_terminal || osc_support )
  {
    oscPrefix();
    putstring (OSC "114" BEL);
    oscPostfix();
    fflush(stdout);
  }
}

//----------------------------------------------------------------------
void FTerm::resetXTermHighlightBackground()
{
  // Reset the highlight background color
  if ( xterm || screen_terminal || urxvt_terminal || osc_support )
  {
    oscPrefix();
    putstringf (OSC "117" BEL);
    oscPostfix();
    fflush(stdout);
  }
}

//----------------------------------------------------------------------
void FTerm::saveColorMap()
{
 // ioctl (0, GIO_CMAP, &map);
}

//----------------------------------------------------------------------
void FTerm::resetColorMap()
{
  char* op = tcap[t_orig_pair].string;
  char* oc = tcap[t_orig_colors].string;

  if ( op )
    putstring (op);
  else if ( oc )
    putstring (oc);
/*else
  {
    dacreg CurrentColors[16] =
    {
      {0x00, 0x00, 0x00}, {0xAA, 0x00, 0x00},
      {0x00, 0xAA, 0x00}, {0xAA, 0x55, 0x00},
      {0x00, 0x00, 0xAA}, {0xAA, 0x00, 0xAA},
      {0x00, 0xAA, 0xAA}, {0xAA, 0xAA, 0xAA},
      {0x55, 0x55, 0x55}, {0xFF, 0x55, 0x55},
      {0x55, 0xFF, 0x55}, {0xFF, 0xFF, 0x55},
      {0x55, 0x55, 0xFF}, {0xFF, 0x55, 0xFF},
      {0x55, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF}
    };
    for (int x=0; x<16; x++)
    {
      map.d[x].red = CurrentColors[x].red;
      map.d[x].green = CurrentColors[x].green;
      map.d[x].blue = CurrentColors[x].blue;
    }
    ioctl (0, PIO_CMAP, &map);
  }*/
  fflush(stdout);
}

//----------------------------------------------------------------------
void FTerm::setPalette (short index, int r, int g, int b)
{
  char* Ic = tcap[t_initialize_color].string;
  char* Ip = tcap[t_initialize_pair].string;

  index = FOptiAttr::vga2ansi(index);

  if ( Ic || Ip )
  {
    int rr, gg, bb;
    const char* color_str = "";

    rr = (r * 1001) / 256;
    gg = (g * 1001) / 256;
    bb = (b * 1001) / 256;

    if ( Ic )
      color_str = tparm(Ic, index, rr, gg, bb, 0, 0, 0, 0, 0);
    else if ( Ip )
      color_str = tparm(Ip, index, 0, 0, 0, rr, gg, bb, 0, 0);
    putstring (color_str);
  }
  else if ( linux_terminal )
  {
/*  // direct vga-register set
    if (  r>=0 && r<256
       && g>=0 && g<256
       && b>=0 && b<256 )
    {
      map.d[index].red = r;
      map.d[index].green = g;
      map.d[index].blue = b;
    }
    ioctl (0, PIO_CMAP, &map); */
  }
  fflush(stdout);
}

//----------------------------------------------------------------------
void FTerm::xtermMouse (bool on)
{
  if ( ! mouse_support )
    return;
  if ( on )
    putstring (CSI "?1001s"   // save old highlight mouse tracking
               CSI "?1000h"   // enable x11 mouse tracking
               CSI "?1002h"   // enable cell motion mouse tracking
               CSI "?1015h"   // enable urxvt mouse mode
               CSI "?1006h"); // enable SGR mouse mode
  else
    putstring (CSI "?1006l"   // disable SGR mouse mode
               CSI "?1015l"   // disable urxvt mouse mode
               CSI "?1002l"   // disable cell motion mouse tracking
               CSI "?1000l"   // disable x11 mouse tracking
               CSI "?1001r"); // restore old highlight mouse tracking
  fflush(stdout);
}


#ifdef F_HAVE_LIBGPM
//----------------------------------------------------------------------
bool FTerm::gpmMouse (bool on)
{
  if ( ! linux_terminal )
    return false;

  if ( openConsole() == 0 )
  {
    if ( ! isConsole() )
      return false;
    closeConsole();
  }

  if ( on )
  {
    Gpm_Connect conn;
    conn.eventMask   = uInt16(~GPM_MOVE);
    conn.defaultMask = GPM_MOVE;
    conn.maxMod      = uInt16(~0);
    conn.minMod      = 0;

    Gpm_Open(&conn, 0);
    switch ( gpm_fd )
    {
      case -1:
        return false;

      case -2:
        Gpm_Close();
        return false;

      default:
        break;
    }
  }
  else
  {
    Gpm_Close();
  }
  return on;
}
#endif  // F_HAVE_LIBGPM

//----------------------------------------------------------------------
void FTerm::setTermXY (register int x, register int y)
{
  int term_width, term_height;
  char* move_str;

  if ( x_term_pos == x && y_term_pos == y )
    return;

  term_width = term->getWidth();
  term_height = term->getHeight();

  if ( x >= term_width )
  {
    y += x / term_width;
    x %= term_width;
  }

  if ( y_term_pos >= term_height )
    y_term_pos = term_height - 1;

  if ( y >= term_height )
    y = term_height - 1;

  if ( cursor_optimisation )
    move_str = opti_move->cursor_move (x_term_pos, y_term_pos, x, y);
  else
    move_str = tgoto(tcap[t_cursor_address].string, x, y);

  if ( move_str )
    appendOutputBuffer(move_str);

  flush_out();

  x_term_pos = x;
  y_term_pos = y;
}

//----------------------------------------------------------------------
void FTerm::setBeep (int Hz, int ms)
{
  if ( ! linux_terminal )
    return;
  // range for frequency: 21-32766
  if ( Hz < 21 || Hz > 32766 )
    return;
  // range for duration:  0-1999
  if ( ms < 0 || ms > 1999 )
    return;
  putstringf ( CSI "10;%d]"
               CSI "11;%d]"
             , Hz, ms );
  fflush(stdout);
}

//----------------------------------------------------------------------
void FTerm::resetBeep()
{
  if ( ! linux_terminal )
    return;
  // default frequency: 750 Hz
  // default duration:  125 ms
  putstring ( CSI "10;750]"
              CSI "11;125]" );
  fflush(stdout);
}

//----------------------------------------------------------------------
void FTerm::beep()
{
  if ( tcap[t_bell].string )
  {
    putstring (tcap[t_bell].string);
    fflush(stdout);
  }
}

//----------------------------------------------------------------------
bool FTerm::hideCursor (bool on)
{
  char *vi, *vs, *ve;

  if ( on == hiddenCursor )
    return hiddenCursor;

  vi = tcap[t_cursor_invisible].string;
  vs = tcap[t_cursor_visible].string;
  ve = tcap[t_cursor_normal].string;

  if ( on )
  {
    if ( vi )
      appendOutputBuffer (vi);
    hiddenCursor = true;  // global
  }
  else
  {
    if ( ve )
      appendOutputBuffer (ve);
    else if ( vs )
      appendOutputBuffer (vs);
    hiddenCursor = false;
  }
  flush_out();

  if ( ! hiddenCursor && linux_terminal )
    setConsoleCursor (consoleCursorStyle);

  return hiddenCursor;
}

//----------------------------------------------------------------------
bool FTerm::isCursorInside()
{
  // Checked whether the cursor is within the visual terminal
  FWidget* w;
  w = static_cast<FWidget*>(this);

  int x = w->getCursorPos().getX();
  int y = w->getCursorPos().getY();

  if ( term->contains(x,y) )
    return true;
  else
    return false;
}

//----------------------------------------------------------------------
void FTerm::setEncoding (std::string enc)
{
  std::map<std::string,fc::encoding>::const_iterator it;

  // available encodings: "UTF8", "VT100", "PC" and "ASCII"
  it = encoding_set->find(enc);

  if ( it != encoding_set->end() )  // found
  {
    Encoding = it->second;

    assert (  Encoding == fc::UTF8
           || Encoding == fc::VT100
           || Encoding == fc::PC
           || Encoding == fc::ASCII );

    // set the new Fputchar function pointer
    switch ( int(Encoding) )
    {
      case fc::UTF8:
        Fputchar = &FTerm::putchar_UTF8;
        break;

      case fc::VT100:
      case fc::PC:
        if ( xterm && utf8_console )
          Fputchar = &FTerm::putchar_UTF8;
        // fall through
      case fc::ASCII:
      default:
        Fputchar = &FTerm::putchar_ASCII;
    }
  }
}

//----------------------------------------------------------------------
std::string FTerm::getEncoding()
{
  std::map<std::string,fc::encoding>::const_iterator it, end;
  end = encoding_set->end();

  for (it = encoding_set->begin(); it != end; ++it )
    if ( it->second == Encoding )
      return it->first;
  return "";
}

//----------------------------------------------------------------------
bool FTerm::setNonBlockingInput (bool on)
{
  if ( on == non_blocking_stdin )
    return non_blocking_stdin;

  if ( on )  // make stdin non-blocking
  {
    stdin_status_flags |= O_NONBLOCK;
    if ( fcntl (stdin_no, F_SETFL, stdin_status_flags) != -1 )
      non_blocking_stdin = true;
  }
  else
  {
    stdin_status_flags &= ~O_NONBLOCK;
    if ( fcntl (stdin_no, F_SETFL, stdin_status_flags) != -1 )
      non_blocking_stdin = false;
  }
  return non_blocking_stdin;
}

//----------------------------------------------------------------------
bool FTerm::setUTF8 (bool on) // UTF-8 (Unicode)
{
  if ( on == utf8_state )
    return utf8_state;

  if ( on )
    utf8_state = true;
  else
    utf8_state = false;

  if ( linux_terminal )
  {
    if ( on )
      putstring (ESC "%G");
    else
      putstring (ESC "%@");
  }
  fflush(stdout);
  return utf8_state;
}

//----------------------------------------------------------------------
bool FTerm::setRawMode (bool on)
{
  if ( on == raw_mode )
    return raw_mode;

  fflush(stdout);

  if ( on )
  {
    // Info under: man 3 termios
    struct termios t;
    tcgetattr (stdin_no, &t);

    /* set + unset flags for raw mode */
    // input mode
    t.c_iflag &= uInt(~(IGNBRK | BRKINT | PARMRK | ISTRIP
                      | INLCR | IGNCR | ICRNL | IXON));
    // output mode
    t.c_oflag &= uInt(~OPOST);

    // local mode
#if DEBUG
    // Exit with ctrl-c only if compiled with "DEBUG" option
    t.c_lflag &= uInt(~(ECHO | ECHONL | ICANON | IEXTEN));
#else
    // Plus disable signals.
    t.c_lflag &= uInt(~(ECHO | ECHONL | ICANON | IEXTEN | ISIG));
#endif

    // control mode
    t.c_cflag &= uInt(~(CSIZE | PARENB));
    t.c_cflag |= uInt(CS8);

    // defines the terminal special characters for noncanonical read
    t.c_cc[VTIME] = 0; // Timeout in deciseconds
    t.c_cc[VMIN]  = 1; // Minimum number of characters

    // set the new termios settings
    tcsetattr (stdin_no, TCSAFLUSH, &t);
    raw_mode = true;
  }
  else
  {
    // restore termios settings
    tcsetattr (stdin_no, TCSAFLUSH, &term_init);
    raw_mode = false;
  }
  return raw_mode;
}

//----------------------------------------------------------------------
FString FTerm::getAnswerbackMsg()
{
  FString answerback = "";

  if ( raw_mode )
  {
    ssize_t n;
    fd_set ifds;
    struct timeval tv;
    char temp[10] = {};

    FD_ZERO(&ifds);
    FD_SET(stdin_no, &ifds);
    tv.tv_sec  = 0;
    tv.tv_usec = 150000;  // 150 ms

    putchar (ENQ[0]);  // send enquiry character
    fflush(stdout);

    // read the answerback message
    if ( select (stdin_no+1, &ifds, 0, 0, &tv) > 0)
    {
      n = read(fileno(stdin), &temp, sizeof(temp)-1);

      if ( n > 0 )
      {
        temp[n] = '\0';
        answerback = temp;
      }
    }
  }
  return answerback;
}

//----------------------------------------------------------------------
FString FTerm::getSecDA()
{
  FString sec_da = "";

  if ( raw_mode )
  {
    ssize_t n;
    fd_set ifds;
    struct timeval tv;
    char temp[16] = {};

    FD_ZERO(&ifds);
    FD_SET(stdin_no, &ifds);
    tv.tv_sec  = 0;
    tv.tv_usec = 550000;  // 150 ms

    // get the secondary device attributes
    putchar (SECDA[0]);
    putchar (SECDA[1]);
    putchar (SECDA[2]);
    putchar (SECDA[3]);

    fflush(stdout);
    usleep(150000);  // min. wait time 150 ms (need for mintty)

    // read the answer
    if ( select (stdin_no+1, &ifds, 0, 0, &tv) > 0 )
    {
      n = read(fileno(stdin), &temp, sizeof(temp)-1);

      if ( n > 0 )
      {
        temp[n] = '\0';
        sec_da = temp;
      }
    }
  }
  return sec_da;
}

//----------------------------------------------------------------------
int FTerm::printf (const wchar_t* format, ...)
{
  assert ( format != 0 );
  const int buf_size = 1024;
  wchar_t buffer[buf_size];
  va_list args;

  va_start (args, format);
  vswprintf (buffer, buf_size, format, args);
  va_end (args);

  FString str(buffer);
  return print(str);
}

//----------------------------------------------------------------------
int FTerm::printf (const char* format, ...)
{
  assert ( format != 0 );
  int   len;
  char  buf[512];
  char* buffer;
  va_list args;

  buffer = buf;
  va_start (args, format);
  len = vsnprintf (buffer, sizeof(buf), format, args);
  va_end (args);

  if ( len >= int(sizeof(buf)) )
  {
    buffer = new char[len+1]();
    va_start (args, format);
    vsnprintf (buffer, uLong(len+1), format, args);
    va_end (args);
  }

  FString str(buffer);
  int ret = print(str);

  if ( buffer != buf )
    delete[] buffer;

  return ret;
}

//----------------------------------------------------------------------
int FTerm::print (const std::wstring& s)
{
  assert ( ! s.empty() );
  return print (FString(s));
}

//----------------------------------------------------------------------
int FTerm::print (FTerm::term_area* area, const std::wstring& s)
{
  assert ( area != 0 );
  assert ( ! s.empty() );
  return print (area, FString(s));
}

//----------------------------------------------------------------------
int FTerm::print (const wchar_t* s)
{
  assert ( s != 0 );
  return print (FString(s));
}

//----------------------------------------------------------------------
int FTerm::print (FTerm::term_area* area, const wchar_t* s)
{
  assert ( area != 0 );
  assert ( s != 0 );
  return print (area, FString(s));
}

//----------------------------------------------------------------------
int FTerm::print (const char* s)
{
  assert ( s != 0 );
  FString str(s);
  return print(str);
}

//----------------------------------------------------------------------
int FTerm::print (FTerm::term_area* area, const char* s)
{
  assert ( area != 0 );
  assert ( s != 0 );
  FString str(s);
  return print(area, str);
}

//----------------------------------------------------------------------
int FTerm::print (const std::string& s)
{
  assert ( ! s.empty() );
  return print (FString(s));
}

//----------------------------------------------------------------------
int FTerm::print (FTerm::term_area* area, const std::string& s)
{
  assert ( area != 0 );
  assert ( ! s.empty() );
  return print (area, FString(s));
}

//----------------------------------------------------------------------
int FTerm::print (FString& s)
{
  assert ( ! s.isNull() );

  term_area* area;
  FWidget* w;
  w = static_cast<FWidget*>(this);
  area = w->getPrintArea();

  if ( ! area )
  {
    FWidget* area_widget = w->getRootWidget();
    area = vdesktop;
    if ( ! area_widget )
      return -1;
  }
  return print (area, s);
}

//----------------------------------------------------------------------
int FTerm::print (FTerm::term_area* area, FString& s)
{
  assert ( ! s.isNull() );
  register int len = 0;
  const wchar_t* p;
  FWidget* area_widget;

  if ( ! area )
    return -1;
  area_widget = area->widget;
  if ( ! area_widget )
    return -1;

  p = s.wc_str();

  if ( p )
  {
    while ( *p )
    {
      int rsh, bsh;

      switch ( *p )
      {
        case '\n':
          cursor->y_ref()++;

        case '\r':
          cursor->x_ref() = 1;
          break;

        case '\t':
          cursor->x_ref() = short ( uInt(cursor->x_ref())
                                  + tabstop
                                  - uInt(cursor->x_ref())
                                  + 1
                                  % tabstop );
          break;

        case '\b':
          cursor->x_ref()--;
          break;

        case '\a':
          beep();
          break;

        default:
        {
          short x = short(cursor->getX());
          short y = short(cursor->getY());

          FOptiAttr::char_data  nc; // next character
          nc.code          = *p;
          nc.fg_color      = next_attribute.fg_color;
          nc.bg_color      = next_attribute.bg_color;
          nc.bold          = next_attribute.bold;
          nc.dim           = next_attribute.dim;
          nc.italic        = next_attribute.italic;
          nc.underline     = next_attribute.underline;
          nc.blink         = next_attribute.blink;
          nc.reverse       = next_attribute.reverse;
          nc.standout      = next_attribute.standout;
          nc.invisible     = next_attribute.invisible;
          nc.protect       = next_attribute.protect;
          nc.crossed_out   = next_attribute.crossed_out;
          nc.dbl_underline = next_attribute.dbl_underline;
          nc.alt_charset   = next_attribute.alt_charset;
          nc.pc_charset    = next_attribute.pc_charset;

          int ax = x - area_widget->getGlobalX();
          int ay = y - area_widget->getGlobalY();

          if (  area
             && ax >= 0 && ay >= 0
             && ax < area->width + area->right_shadow
             && ay < area->height + area->bottom_shadow )
          {
            FOptiAttr::char_data* ac; // area character
            int line_len = area->width + area->right_shadow;
            ac = &area->text[ay * line_len + ax];

            if ( *ac != nc )
            {
              memcpy (ac, &nc, sizeof(nc));

              if ( ax < short(area->changes[ay].xmin) )
                area->changes[ay].xmin = uInt(ax);
              if ( ax > short(area->changes[ay].xmax) )
                area->changes[ay].xmax = uInt(ax);
            }
          }
          cursor->x_ref()++;
        }
      }
      rsh = area->right_shadow;
      bsh = area->bottom_shadow;
      const FRect& area_geometry = area_widget->getGeometryGlobal();

      if ( cursor->x_ref() > area_geometry.getX2()+rsh )
      {
        cursor->x_ref() = short(area_widget->getGlobalX());
        cursor->y_ref()++;
      }
      if ( cursor->y_ref() > area_geometry.getY2()+bsh )
      {
        cursor->y_ref()--;
        break;
      }
      p++;
      len++;
    } // end of while
    updateVTerm (area);
  }
  return len;
}

//----------------------------------------------------------------------
int FTerm::print (register int c)
{
  term_area* area;
  FWidget* w;
  w = static_cast<FWidget*>(this);
  area = w->getPrintArea();

  if ( ! area )
  {
    FWidget* area_widget = w->getRootWidget();
    area = vdesktop;
    if ( ! area_widget )
      return -1;
  }
  return print (area, c);
}

//----------------------------------------------------------------------
int FTerm::print (FTerm::term_area* area, register int c)
{
  FOptiAttr::char_data nc; // next character
  FWidget* area_widget;
  int rsh, bsh, ax, ay;
  short x, y;

  if ( ! area )
    return -1;

  nc.code          = c;
  nc.fg_color      = next_attribute.fg_color;
  nc.bg_color      = next_attribute.bg_color;
  nc.bold          = next_attribute.bold;
  nc.dim           = next_attribute.dim;
  nc.italic        = next_attribute.italic;
  nc.underline     = next_attribute.underline;
  nc.blink         = next_attribute.blink;
  nc.reverse       = next_attribute.reverse;
  nc.standout      = next_attribute.standout;
  nc.invisible     = next_attribute.invisible;
  nc.protect       = next_attribute.protect;
  nc.crossed_out   = next_attribute.crossed_out;
  nc.dbl_underline = next_attribute.dbl_underline;
  nc.alt_charset   = next_attribute.alt_charset;
  nc.pc_charset    = next_attribute.pc_charset;

  x = short(cursor->getX());
  y = short(cursor->getY());

  area_widget = area->widget;
  if ( ! area_widget )
    return -1;

  ax = x - area_widget->getGlobalX();
  ay = y - area_widget->getGlobalY();

  if (  ax >= 0 && ay >= 0
     && ax < area->width + area->right_shadow
     && ay < area->height + area->bottom_shadow )
  {
    FOptiAttr::char_data* ac; // area character
    int line_len = area->width + area->right_shadow;
    ac = &area->text[ay * line_len + ax];

    if ( *ac != nc )
    {
      memcpy (ac, &nc, sizeof(nc));

      if ( ax < short(area->changes[ay].xmin) )
        area->changes[ay].xmin = uInt(ax);
      if ( ax > short(area->changes[ay].xmax) )
        area->changes[ay].xmax = uInt(ax);
    }
  }
  cursor->x_ref()++;

  rsh = area->right_shadow;
  bsh = area->bottom_shadow;
  const FRect& area_geometry = area_widget->getGeometryGlobal();

  if ( cursor->x_ref() > area_geometry.getX2()+rsh )
  {
    cursor->x_ref() = short(area_widget->getGlobalX());
    cursor->y_ref()++;
  }
  if ( cursor->y_ref() > area_geometry.getY2()+bsh )
  {
    cursor->y_ref()--;
    updateVTerm (area);
    return -1;
  }
  updateVTerm (area);
  return 1;
}

//----------------------------------------------------------------------
inline void FTerm::newFontChanges (FOptiAttr::char_data*& next_char)
{
  // NewFont special cases
  if ( isNewFont() )
  {
    switch ( next_char->code )
    {
      case fc::LowerHalfBlock:
        next_char->code = fc::UpperHalfBlock;
        // fall through
      case fc::NF_rev_left_arrow2:
      case fc::NF_rev_right_arrow2:
      case fc::NF_rev_border_corner_upper_right:
      case fc::NF_rev_border_line_right:
      case fc::NF_rev_border_line_vertical_left:
      case fc::NF_rev_border_corner_lower_right:
      case fc::NF_rev_up_arrow2:
      case fc::NF_rev_down_arrow2:
      case fc::NF_rev_up_arrow1:
      case fc::NF_rev_down_arrow1:
      case fc::NF_rev_left_arrow1:
      case fc::NF_rev_right_arrow1:
      case fc::NF_rev_menu_button1:
      case fc::NF_rev_menu_button2:
      case fc::NF_rev_up_pointing_triangle1:
      case fc::NF_rev_down_pointing_triangle1:
      case fc::NF_rev_up_pointing_triangle2:
      case fc::NF_rev_down_pointing_triangle2:
      case fc::NF_rev_menu_button3:
      case fc::NF_rev_border_line_right_and_left:
        // swap foreground and background color
        std::swap (next_char->fg_color, next_char->bg_color);
        break;

      default:
        break;
    }
  }
}

//----------------------------------------------------------------------
inline void FTerm::charsetChanges (FOptiAttr::char_data*& next_char)
{
  if ( Encoding == fc::UTF8 )
    return;

  uInt code = uInt(next_char->code);
  uInt ch = charEncode(code);

  if ( ch != code )
  {
    if ( ch == 0 )
    {
      next_char->code = int(charEncode(code, fc::ASCII));
      return;
    }
    next_char->code = int(ch);

    if ( Encoding == fc::VT100 )
      next_char->alt_charset = true;
    else if ( Encoding == fc::PC )
    {
      next_char->pc_charset = true;

      if ( xterm && utf8_console && ch < 0x20 )  // Character 0x00..0x1f
        next_char->code = int(charEncode(code, fc::ASCII));
    }
  }
}

//----------------------------------------------------------------------
inline void FTerm::appendCharacter (FOptiAttr::char_data*& next_char)
{
  newFontChanges (next_char);
  charsetChanges (next_char);

  appendAttributes (next_char);
  appendOutputBuffer (next_char->code);
}

//----------------------------------------------------------------------
inline void FTerm::appendAttributes (FOptiAttr::char_data*& next_attr)
{
  char* attr_str;
  FOptiAttr::char_data* term_attr = &term_attribute;

  // generate attribute string for the next character
  attr_str = opti_attr->change_attribute (term_attr, next_attr);

  if ( attr_str )
    appendOutputBuffer (attr_str);
}

//----------------------------------------------------------------------
int FTerm::appendLowerRight (FOptiAttr::char_data*& screen_char)
{
  char* SA = tcap[t_enter_am_mode].string;
  char* RA = tcap[t_exit_am_mode].string;

  if ( ! automatic_right_margin )
  {
    appendCharacter (screen_char);
  }
  else if ( SA && RA )
  {
    appendOutputBuffer (RA);
    appendCharacter (screen_char);
    appendOutputBuffer (SA);
  }
  else
  {
    int x, y;
    char* IC = tcap[t_parm_ich].string;
    char* im = tcap[t_enter_insert_mode].string;
    char* ei = tcap[t_exit_insert_mode].string;
    char* ip = tcap[t_insert_padding].string;
    char* ic = tcap[t_insert_character].string;

    x = term->getWidth() - 2;
    y = term->getHeight() - 1;
    setTermXY (x, y);
    appendCharacter (screen_char);
    x_term_pos++;

    setTermXY (x, y);
    screen_char--;

    if ( IC )
    {
      appendOutputBuffer (tparm(IC, 1));
      appendCharacter (screen_char);
    }
    else if ( im && ei )
    {
      appendOutputBuffer (im);
      appendCharacter (screen_char);
      if ( ip )
        appendOutputBuffer (ip);
      appendOutputBuffer (ei);
    }
    else if ( ic )
    {
      appendOutputBuffer (ic);
      appendCharacter (screen_char);
      if ( ip )
        appendOutputBuffer (ip);
    }
  }
  return screen_char->code;
}

//----------------------------------------------------------------------
inline void FTerm::appendOutputBuffer (std::string& s)
{
  const char* c_string = s.c_str();
  tputs (c_string, 1, appendOutputBuffer);
}

//----------------------------------------------------------------------
inline void FTerm::appendOutputBuffer (const char* s)
{
  tputs (s, 1, appendOutputBuffer);
}

//----------------------------------------------------------------------
int FTerm::appendOutputBuffer (int ch)
{
  output_buffer->push(ch);
  if ( output_buffer->size() >= 2048 )
    flush_out();
  return ch;
}

//----------------------------------------------------------------------
void FTerm::flush_out()
{
  while ( ! output_buffer->empty() )
  {
    Fputchar (output_buffer->front());
    output_buffer->pop();
  }
  fflush(stdout);
}

//----------------------------------------------------------------------
void FTerm::putstringf (const char* format, ...)
{
  assert ( format != 0 );
  char  buf[512];
  char* buffer;
  va_list args;

  buffer = buf;
  va_start (args, format);
  vsnprintf (buffer, sizeof(buf), format, args);
  va_end (args);

  tputs (buffer, 1, putchar);
}

//----------------------------------------------------------------------
inline void FTerm::putstring (const char* s, int affcnt)
{
  tputs (s, affcnt, putchar);
}

//----------------------------------------------------------------------
int FTerm::putchar_ASCII (register int c)
{
  if ( putchar(char(c)) == EOF )
    return 0;
  else
    return 1;
}

//----------------------------------------------------------------------
int FTerm::putchar_UTF8 (register int c)
{
  if (c < 0x80)
  {
    // 1 Byte (7-bit): 0xxxxxxx
    putchar (c);
    return 1;
  }
  else if (c < 0x800)
  {
    // 2 byte (11-bit): 110xxxxx 10xxxxxx
    putchar (0xc0 | (c >> 6) );
    putchar (0x80 | (c & 0x3f) );
    return 2;
  }
  else if (c < 0x10000)
  {
    // 3 byte (16-bit): 1110xxxx 10xxxxxx 10xxxxxx
    putchar (0xe0 | (c >> 12) );
    putchar (0x80 | ((c >> 6) & 0x3f) );
    putchar (0x80 | (c & 0x3f) );
    return 3;
  }
  else if (c < 0x200000)
  {
    // 4 byte (21-bit): 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    putchar (0xf0 | (c >> 18) );
    putchar (0x80 | ((c >> 12) & 0x3f) );
    putchar (0x80 | ((c >> 6) & 0x3f) );
    putchar (0x80 | (c & 0x3f));
    return 4;
  }
  else
    return EOF;
}

//----------------------------------------------------------------------
int FTerm::UTF8decode(char* utf8)
{
  register int ucs=0;

  for (register int i=0; i < int(strlen(utf8)); ++i)
  {
    register uChar ch = uChar(utf8[i]);

    if ((ch & 0xc0) == 0x80)
    {
      // byte 2..4 = 10xxxxxx
      ucs = (ucs << 6) | (ch & 0x3f);
    }
    else if (ch < 128)
    {
      // byte 1 = 0xxxxxxx (1 byte mapping)
      ucs = ch & 0xff;
    }
    else if ((ch & 0xe0) == 0xc0)
    {
      // byte 1 = 110xxxxx (2 byte mapping)
      ucs = ch & 0x1f;
    }
    else if ((ch & 0xf0) == 0xe0)
    {
      // byte 1 = 1110xxxx (3 byte mapping)
      ucs = ch & 0x0f;
    }
    else if ((ch & 0xf8) == 0xf0)
    {
      // byte 1 = 11110xxx (4 byte mapping)
      ucs = ch & 0x07;
    }
    else
    {
      // error
      ucs = EOF;
    }
  }
  return ucs;
}
