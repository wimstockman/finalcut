#include <final/final.h>
#include <regex>
#include <string>
#include <fstream>
#include <iostream>

using namespace finalcut;

class QuickForm : public FDialog
{
	public:
		explicit QuickForm (FWidget* parent ,std::vector<std::string> &data ) : FDialog(parent)
    		{
			//create quickform
			this->setText("QuickForm");
			this->setGeometry (FPoint(1, 1), FSize(this->getDesktopWidth(), this->getDesktopHeight()));
			this->quit_btn.setGeometry (FPoint((this->getWidth()/2-5),(this->getHeight()-3)), FSize(10, 1));
			this->next_btn.setGeometry (FPoint((5),1), FSize(3, 1));
			this->prev_btn.setGeometry (FPoint((1),1), FSize(3, 1));
			this->data = &data;
			this->data->push_back("Tester");

			init_records_tabel(data);
<<<<<<< HEAD
//			for_each(records_vctr.begin(),records_vctr.end(),[](std::string &s){ std::cerr<<s<<"lambda \n";});
=======
			//for_each(records_vctr.begin(),records_vctr.end(),[](std::string &s){ std::cerr<<s<<"lambda \n";});
>>>>>>> 79c1561d031532d0bd227f41d2fb76f972ce2279
			this->prev_btn.addCallback("clicked", F_METHOD_CALLBACK (this,&QuickForm::cb_prev), nullptr);
			this->next_btn.addCallback("clicked", F_METHOD_CALLBACK (this,&QuickForm::cb_next), nullptr);
			this->quit_btn.addCallback("clicked", F_METHOD_CALLBACK (this,&QuickForm::cb_quit), nullptr);
			this->total_records = data.size()-1 ;
			std::cerr<<this->total_records<<"Total\n";
			//this->create_FLineEdits(this->tokenize_record(records_vctr[active_record]));
			this->create_FLineEdits(this->records_tabel[this->active_record]);
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
		
		char OFS = " ";
		std::regex delim_re{"\\s+"};
		
		//Create Buttons
		FButton quit_btn{"&Save en Quit", this};
		FButton prev_btn{fc::BlackLeftPointingTriangle, this};
		FButton next_btn{fc::BlackRightPointingTriangle, this};

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

		void cb_quit (FWidget*, FDataPtr)
		{
			
			for(int i=0;i<this->records_tabel.size();i++){
					std::string helper="";
				for(int j=0;j<this->records_tabel[i].size();j++)
					{
						helper+=this->records_tabel[i][j]<<OFS;
					}
				this->data[i] = helper;	
				}
			for_each(records_vctr.begin(),records_vctr.end(),[](std::string &s){ std::cerr<<s<<"lambda \n";});
			this->quit();
		}
		void update_record()
		{
			std::string a = "hello";
			std::cerr<<"Active Record:"<<this->active_record<<" Fields vector:"<<this->Fields_vctr.size()<<"record tabel:"<<this->records_tabel[this->active_record].size()<<"\n";
		  for(int i=0;i<this->records_tabel[this->active_record].size();i++)
				{
			this->records_tabel[this->active_record][i]=this->Fields_vctr[i]->getText().toString();
<<<<<<< HEAD
			std::cerr<<this->Fields_vctr[i]->getText()<<i<<" : ";
=======
			//std::cerr<<this->Fields_vctr[i]->getText()<<i<<" : ";
>>>>>>> 79c1561d031532d0bd227f41d2fb76f972ce2279
			}
		}
		void cb_next (FWidget*, FDataPtr){
			this->update_record();
			if (this->active_record < this->total_records)	this->active_record++;
			this->clear_FLineEdits();
			this->create_FLineEdits(this->records_tabel[this->active_record]);
		}

		void cb_prev (FWidget*, FDataPtr){
			this->update_record();
			if (this->active_record > 0)this->active_record--;
			this->clear_FLineEdits();
			this->create_FLineEdits(this->records_tabel[this->active_record]);
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
<<<<<<< HEAD
			 Fields_vctr.erase(Fields_vctr.begin(),Fields_vctr.end());
=======
			this->Fields_vctr.erase (Fields_vctr.begin(),Fields_vctr.end());

>>>>>>> 79c1561d031532d0bd227f41d2fb76f972ce2279
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
int init (int argc, char* argv[], std::vector<std::string>*v)
{

	FApplication app(argc, argv,false);
	QuickForm dialog(&app,*v);
	app.setMainWidget(&dialog);
	dialog.show();
	return app.exec();
}

int main (int argc, char* argv[])
{
	std::vector<std::string>v {};
	v = processparameters(argc,argv); for (int i=0;i<v.size();i++) std::cerr<<v[i]<<i<<"vector\n";
	argc = 0;
	argv = nullptr;
	int fd_stdin{fileno(stdin)};
	std::cerr<<fd_stdin<<"fd stdin \n";
	close(fd_stdin);
	fd_stdin = open("/dev/tty",O_RDWR);
	int stdoutBack = dup(1);
	close(1);
	int output = open("/dev/tty",O_RDWR);
	init(argc,argv,&v);
	dup2(stdoutBack,1);
	std::cout<<"Hello Worldttt \n";
	for_each(v.begin(),v.end(),[](std::string &s){ std::cout<<s<<"\n";});
	return 0 ;

}


