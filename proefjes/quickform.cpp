#include <final/final.h>
#include <regex>
#include <string>
#include <fstream>
#include <iostream>

using namespace finalcut;

class QuickForm : public FDialog
{
	public:
		explicit QuickForm (FWidget* parent ,std::vector<std::string> data ) : FDialog(parent)
    		{
			//create quickform
			this->setText("QuickForm");
			this->setGeometry (FPoint(1, 1), FSize(this->getDesktopWidth(), this->getDesktopHeight()));
			this->quit_btn.setGeometry (FPoint((this->getWidth()/2-5),(this->getHeight()-3)), FSize(10, 1));
			this->next_btn.setGeometry (FPoint((5),1), FSize(3, 1));
			this->prev_btn.setGeometry (FPoint((1),1), FSize(3, 1));
			this->records_vctr = data;
			for_each(records_vctr.begin(),records_vctr.end(),[](std::string &s){ std::cerr<<s<<"lambda \n";});
			for (int i=0;i<data.size();i++)
				std::cerr<<data[i]<<i<<"object\n";		
			
	this->prev_btn.addCallback("clicked", F_METHOD_CALLBACK (this,&QuickForm::cb_prev), nullptr);
	this->next_btn.addCallback("clicked", F_METHOD_CALLBACK (this,&QuickForm::cb_next), nullptr);
	this->quit_btn.addCallback("clicked", F_METHOD_CALLBACK (this,&QuickForm::cb_quit), nullptr);
			this->total_records = (records_vctr.size()-1);
			this->create_FLineEdits(this->tokenize_record(records_vctr[active_record]));
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
		
		//Create Buttons
		FButton quit_btn{"&Quit", this};
		FButton prev_btn{fc::BlackLeftPointingTriangle, this};
		FButton next_btn{fc::BlackRightPointingTriangle, this};

		//Declare Vector to hold the fields
		std::vector<FLineEdit*> Fields_vctr;
		std::vector<std::string> records_vctr;
		
		std::vector<std::string>tokenize_record(std::string record)
		{
		  const std::regex delim_re("\\s+");
		  return std::vector<std::string> {std::sregex_token_iterator{record.begin(),record.end(),delim_re,-1},std::sregex_token_iterator{}};
		}

		void cb_quit (FWidget*, FDataPtr)
		{
			this->quit();
		}

		void cb_next (FWidget*, FDataPtr){
			if (this->active_record < this->total_records)	this->active_record++;
			this->clear_FLineEdits();
			this->create_FLineEdits(this->tokenize_record(records_vctr[active_record]));
		}

		void cb_prev (FWidget*, FDataPtr){
			if (this->active_record > 0)this->active_record--;
			this->clear_FLineEdits();
			this->create_FLineEdits(this->tokenize_record(records_vctr[active_record]));
		}

		void create_FLineEdits(std::vector<std::string> record)
			{
			int active_posy = 0;
			int active_posx = posx;
				try {
			for(int i=0;i< record.size();i++){
				FLineEdit* field  = new FLineEdit{record[i],this};
				int active_posy = (i*(this->vpadding+this->height)+this->posy);
					if (active_posy > this->form_height) {throw std::range_error("To many fields for the Form");}

					field->setGeometry(FPoint(this->posx,active_posy),FSize(20,1));
				    
			//		field->setLabelText(record[i]);
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
			for(int i=0;i < this->Fields_vctr.size();i++){
				this->Fields_vctr[i]->clear();
				this->Fields_vctr[i]->hide();

				}
			for(int i=0;i < this->Fields_vctr.size();i++){
				this->Fields_vctr.pop_back();
				} 

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

int main (int argc, char* argv[])
{
	std::vector<std::string> v {};
	v = processparameters(argc,argv); for (int i=0;i<v.size();i++) std::cerr<<v[i]<<i<<"vector\n";
	argc = 0;
	argv = nullptr;
	int fd_stdin{fileno(stdin)};
	std::cerr<<fd_stdin<<"\n";
	close(fd_stdin);
	fd_stdin = open("/dev/tty",O_RDWR);
	std::cerr<<fd_stdin<<"\n";
	FApplication app(argc, argv);
	QuickForm dialog(&app,v);
	app.setMainWidget(&dialog);
	dialog.show();
	return app.exec();
}


