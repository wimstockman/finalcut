#include <final/final.h>
#include <regex>
#include <fstream>

using namespace finalcut;

std::string destination{};
std::string source{};
std::string delimiter{"\\s+"};

class QuickForm : public FDialog
{
	public:
		explicit QuickForm (FWidget* parent ,std::vector<std::string> &data ) : FDialog(parent)
    		{
			//create quickform
			this->setText("QuickForm");
			this->setGeometry (FPoint(1, 1), FSize(this->getDesktopWidth(), this->getDesktopHeight()));
			
			//create buttons
			
			this->exit_btn.setGeometry (FPoint((this->getWidth()/2-20),(this->getHeight()-3)), FSize(20, 1));
			this->exit_btn.addCallback("clicked", F_METHOD_CALLBACK (this,&QuickForm::cb_exit), nullptr);
			
			this->save_and_exit_btn.setGeometry (FPoint((this->getWidth()/2+1),(this->getHeight()-3)), FSize(20, 1));
			this->save_and_exit_btn.addCallback("clicked", F_METHOD_CALLBACK (this,&QuickForm::cb_save_and_exit), nullptr);
			
			
			this->first_btn.setGeometry (FPoint((1),1), FSize(3, 1));
			this->first_btn.addCallback("clicked", F_METHOD_CALLBACK (this,&QuickForm::cb_first), nullptr);
			this->first_btn.setShadow(true);			
			this->prev_btn.setGeometry (FPoint((5),1), FSize(3, 1));
			this->prev_btn.addCallback("clicked", F_METHOD_CALLBACK (this,&QuickForm::cb_prev), nullptr);
			this->prev_btn.setShadow(true);			
			this->next_btn.setGeometry (FPoint((9),1), FSize(3, 1));
			this->next_btn.addCallback("clicked", F_METHOD_CALLBACK (this,&QuickForm::cb_next), nullptr);
			this->next_btn.setShadow(true);			
			this->last_btn.setGeometry (FPoint((13),1), FSize(3, 1));
			this->last_btn.addCallback("clicked", F_METHOD_CALLBACK (this,&QuickForm::cb_last), nullptr);
			this->last_btn.setShadow(true);			
			this->recnumber.setGeometry (FPoint((20),1), FSize(3, 1));
			//get a reference to our original data so at the end we can change the data through this reference.
			this->data = &data;

			init_records_tabel(data); //initialise our table (split the strings and create records with fields)

			this->total_records = data.size()-1 ; //get the total records for our record pointer

			this->create_FLineEdits(this->records_tabel[this->active_record]);  //create the first record fields on the form
			this->recnumber.setText(std::to_string(active_record)+"|"+std::to_string(total_records));
		}
	private:
		int posx = 1;
		int posy = 3;
		int hpadding = 1;
		int vpadding = 1 ;
		int width = 20;
		int height = 1;
		int form_width = 50;
		int form_height	= 40;
		int active_record = 0;
		int total_records = 0;
		
		int min_fieldsize = 10;
		int max_fieldsize{form_width - 5};
		std::string OFS = " ";
		std::regex delim_re{"\\s+"};
		
		//Create Buttons
		FButton exit_btn{"&Quit", this};
		FButton save_and_exit_btn{"&Save en Quit", this};
		FButton prev_btn{fc::BlackLeftPointingTriangle, this};
		FButton first_btn{fc::BlackLeftPointingPointer, this};
		FButton last_btn{fc::BlackRightPointingPointer, this};
		FButton next_btn{fc::BlackRightPointingTriangle, this};
		FLabel recnumber{std::to_string(active_record)+"|"+std::to_string(total_records),this};
		//Declare Vector to hold the fields
		std::vector<FLineEdit*> Fields_vctr;
		std::vector<std::string> records_vctr;
		std::vector<std::vector<std::string> > records_tabel;
		std::vector<std::string>* data; 
		
		void init_records_tabel(std::vector<std::string> data)
		{
		for(int i=0;i<data.size();i++)
			{
			std::vector<std::string> v {std::sregex_token_iterator{data[i].begin(),data[i].end(),delim_re,-1},std::sregex_token_iterator{}};
			this->records_tabel.push_back(v);
			}
			
		}
		void cb_exit(FWidget*, FDataPtr)
		{
		close();
		 //this->quit();
		}

		void cb_save_and_exit (FWidget*, FDataPtr)
		{
			this->update_record();
			data->erase(data->begin(),data->end());
			for(int i=0;i<this->records_tabel.size();i++){
					std::string helper="";
				for(int j=0;j<this->records_tabel[i].size();j++)
					{
					helper+=this->records_tabel[i][j]+OFS;
					}
			//std::cerr<<helper<<"\nHellaw\n";
			this->data->push_back(helper);
				}
		this->close();
		}
		
		void update_record()
		{
		  for(int i=0;i<this->Fields_vctr.size();i++)
				{
				this->records_tabel[this->active_record][i]=this->Fields_vctr[i]->getText().toString();
				}
		}

		void cb_first (FWidget*, FDataPtr)
		{
			this->update_record();
			this->active_record = 0;
			this->recnumber.setText(std::to_string(active_record)+"|"+std::to_string(total_records));
			this->clear_FLineEdits();
			this->create_FLineEdits(this->records_tabel[this->active_record]);
		}
		void cb_next (FWidget*, FDataPtr)
		{
			this->update_record();
			if (this->active_record < this->total_records)	this->active_record++;
			this->recnumber.setText(std::to_string(active_record)+"|"+std::to_string(total_records));
			this->clear_FLineEdits();
			this->create_FLineEdits(this->records_tabel[this->active_record]);
		}
		void cb_last (FWidget*, FDataPtr)
		{
			this->update_record();
			this->active_record = this->total_records;
			this->recnumber.setText(std::to_string(active_record)+"|"+std::to_string(total_records));
			this->clear_FLineEdits();
			this->create_FLineEdits(this->records_tabel[this->active_record]);
		}

		void cb_prev (FWidget*, FDataPtr)
		{
			this->update_record();
			if (this->active_record > 0)this->active_record--;
			this->recnumber.setText(std::to_string(active_record)+"|"+std::to_string(total_records));
			this->clear_FLineEdits();
			this->create_FLineEdits(this->records_tabel[this->active_record]);
		}
		void onClose( finalcut::FCloseEvent *ev)
		{
		  finalcut::FApplication::closeConfirmationDialog(this,ev);
		}
		void create_FLineEdits(std::vector<std::string> record)
		{
			int active_posy = 0;
			int active_posx = posx;
			try
			{
				for(int i=0;i< record.size();i++)
				{
					FLineEdit* field  = new FLineEdit{record[i],this};
					int fieldsize = record[i].length();
					if (fieldsize < this->min_fieldsize) fieldsize = this->min_fieldsize;
					if (fieldsize > this->max_fieldsize) fieldsize = this->max_fieldsize;
					int active_posy = (i*(this->vpadding+this->height)+this->posy);
					if (active_posy > this->form_height) {throw std::range_error("Too many fields for the Form");}
					field->setGeometry(FPoint(this->posx,active_posy),FSize(fieldsize,1));
					field->redraw();
					this->Fields_vctr.push_back(field);
				}
			}
			catch( std::range_error& e)
			{
			std::cerr<<e.what();
			this->quit();
			}
				
			this->redraw();
			this->show();
			
		}

		void clear_FLineEdits()
		{
			for(int i=0;i < this->Fields_vctr.size();i++)
			{
				this->Fields_vctr[i]->clear();
				this->Fields_vctr[i]->hide();
			}
			Fields_vctr.erase(Fields_vctr.begin(),Fields_vctr.end());
		}
};

std::vector<std::string>readData(std::istream& in)
	{
		std::string line;
		std::vector<std::string>v {};
		while(std::getline(in,line))
			{
			 v.push_back(line);
			}
		return v;
	}
int writeData(std::vector<std::string>*v)
	{
	if (destination != "")
	{ 
		std::ofstream outputfile;
  		outputfile.open (destination);
		for_each(v->begin(),v->end(),[&outputfile](std::string &s){ outputfile<<s<<"\n";});
  		outputfile.close();
	}
	else
	{
		for_each(v->begin(),v->end(),[](std::string &s){ std::cout<<s<<"\n";});
	}
  	return 0;	
	}
std::vector<std::string> processparameters(const int& argc, char** argv)
	{
	if( argc > 1)
		{
			std::ifstream ifile(argv[1]);
			if (ifile)
				{
				 return readData(ifile);
				}
			else 
				{
				//deal with file errors
				}
		}
	return readData(std::cin);
	
	}
int init (int argc, char* argv[], std::vector<std::string>*v)
{

	FApplication app(argc, argv,false);
	QuickForm dialog(&app,*v);
	app.setMainWidget(&dialog);
	dialog.show();
	return app.exec();
}

void usage()
{
std::cout<<"Usage : quickform [-i inputfile -o outputfile -D delimeter]\n \
if -o option is ommitted sents output to stdout \n \
Example cat inputfile | quickform > outputfile: takes stdin and outputs to stdout\n \
-D delimiter is default whitespaces regex s+ \n ";
}
void cmd_options(const int& argc, char* argv[])
{
	//Interpret command line options
	const char* const short_options ="o:i";
	static struct option long_options[] =
	{
	 {C_STR("destination"),	required_argument,nullptr,'o'},
	 {C_STR("source"),	required_argument,nullptr,'i'},
	 {0,0,0,0}
	};
	while (true)
	{
	 const auto opt = getopt_long(argc,argv,short_options,long_options,nullptr);

		if (-1 == opt) break;
		switch (opt)
		{
		case 'o':
			std::cout<< "Destination is:" << optarg;
			destination = optarg;
			break;
		case 'i':
			std::cout<< "Input is: "<< optarg;
			source = optarg;
			break;
		case 'h':
		case '?':
		default:
			usage();
			exit(1);
		}	

	}
	
}


int main (int argc, char* argv[])
{
	cmd_options(argc,argv);
	std::vector<std::string>v {};
	int fd_stdin{fileno(stdin)};
	if (isatty(fd_stdin) || argc < 1) //if no input exists, exit and return usage 
	{usage();
	 return 1;}
	v = processparameters(argc,argv); 
	close(fd_stdin);
	fd_stdin = open("/dev/tty",O_RDWR);
	int stdoutBack = dup(1);
	close(1);
	int output = open("/dev/tty",O_RDWR);
	init(argc,argv,&v);
	dup2(stdoutBack,1);
	writeData(&v);
	return 0 ;

}

