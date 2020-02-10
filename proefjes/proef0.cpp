#include <final/final.h>
#include <mysql.h>


using namespace finalcut;

// Defining Constant Variables
#define SERVER "localhost"
#define USER "root"
#define PASSWORD "13gold"
#define DATABASE "FinalCutTryOut"
class Form : public FDialog
{
  public:
    explicit Form (FWidget* parent = nullptr)
      : FDialog(parent)
    {
	this->setText ("Callback method");
	this->setGeometry (FPoint(1, 1), FSize(50, 40));
	this->button.setGeometry (FPoint((this->getWidth()/2-5),(this->getHeight()-3)), FSize(10, 1));
	this->next_btn.setGeometry (FPoint((9),3), FSize(3, 1));
	this->prev_btn.setGeometry (FPoint((5),3), FSize(3, 1));


	this->company_nr_field.setGeometry (FPoint(9, 6), FSize(10, 1));
	this->name_field.setGeometry (FPoint(9, 8), FSize(30, 1));	
	this->email_field.setGeometry (FPoint(9, 10), FSize(30, 1));	
	this->city_field.setGeometry (FPoint(9, 12), FSize(30, 1));	
	this->st_field.setGeometry (FPoint(9, 14), FSize(30, 1));	
	this->c_field.setGeometry (FPoint(9, 16), FSize(30, 1));	

	this->company_nr_field.setLabelText (L"Client&Id");
	this->name_field.setLabelText (L"&Name");
	this->email_field.setLabelText (L"&Email");
	this->city_field.setLabelText (L"&City");
	this->st_field.setLabelText (L"&State");
	this->c_field.setLabelText (L"&Country");
      // Connect the button signal "clicked" with the callback method
      button.addCallback ( "clicked", F_METHOD_CALLBACK (this,&Form::cb_test), nullptr);
	this->next_btn.addCallback("clicked", F_METHOD_CALLBACK (this,&Form::cb_next), nullptr);
	this->prev_btn.addCallback("clicked", F_METHOD_CALLBACK (this,&Form::cb_prev), nullptr);
	this->get_companies();
    }

  private:

	std::vector<MYSQL_ROW>rows;
	unsigned int active_row=0;
	unsigned int numrows=0; 
// Create input fields
	FLineEdit company_nr_field{"1234",this};
	FLineEdit name_field {this};
	FLineEdit email_field {this};
	FLineEdit city_field {this};
	FLineEdit st_field {this};
	FLineEdit c_field {this};

    FButton button{"&Quit", this};
    FButton prev_btn{fc::BlackLeftPointingTriangle, this};
    FButton next_btn{fc::BlackRightPointingTriangle, this};
    void cb_test(FWidget*, FDataPtr){ 
	FLineEdit test_field{this}; 
	test_field.setGeometry(FPoint(9,20),FSize(20,1));
	test_field.setLabelText("Teste");
	this->redraw();
	this->show();
//	this->close();
	}

    void cb_next(FWidget*, FDataPtr)
	{
	this->active_row++;
	if (this->active_row < numrows) this->setfield_data();
	else this->active_row--;
	} 
    void cb_prev(FWidget*, FDataPtr)
	{
	if (this->active_row > 0) 
		{
		this->active_row--;
		this->setfield_data();
		}
	 }
	void get_companies()
	{
   MYSQL *connect;
   connect = mysql_init(NULL);

   if (!connect)
   {
       std::cout << "Mysql Initialization Failed";
   }

   connect = mysql_real_connect(connect, SERVER, USER, PASSWORD, DATABASE, 0,NULL,0);

   if (connect)
   {
       std::cout << "Connection Succeeded\n";
   }
   else
   {
       std::cout << "Connection Failed\n";
   }

   MYSQL_RES *res_set;
   MYSQL_ROW row;

   // Replace MySQL query with your query

   mysql_query (connect,"SELECT * FROM CompanyAdres ");

   res_set=mysql_store_result(connect);
   numrows = mysql_num_rows(res_set);
	//Get First Line
	while ((row=mysql_fetch_row(res_set)) !=NULL) 
   {		rows.push_back(row);   }
	this->setfield_data();
   mysql_close (connect);
}
void setfield_data(){
	MYSQL_ROW row=rows[active_row];
       this->company_nr_field.setText(row[0]);
	this->name_field.setText(row[1]);
	this->email_field.setText(row[2]);
	this->city_field.setText(row[3]);
	this->st_field.setText(row[4]);
	this->c_field.setText(row[5]);
	this->redraw();

	}
};

int main (int argc, char* argv[])
{
  FApplication app(argc, argv);
  Form dialog(&app);
  app.setMainWidget(&dialog);
  dialog.show();
  return app.exec();
}
