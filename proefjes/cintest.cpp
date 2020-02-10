#include <iostream>
#include <fstream>
#include <string>

void readData(std::istream& in)
{
   std::string lijn;
   // Do the necessary work to read the data.
while(std::getline(in,lijn)){

   std::cout << "Hello," << lijn<<"\n";
}
}
int main(int argc, char** argv)
{
   if ( argc > 1 )
   {
      // The input file has been passed in the command line.
      // Read the data from it.
      std::ifstream ifile(argv[1]);
      if ( ifile )
      {
         readData(ifile);
      }
      else
      {
         // Deal with error condition
      }
   }
   else
   {
      // No input file has been passed in the command line.
      // Read the data from stdin (std::cin).
      readData(std::cin);
   }

   // Do the needful to process the data.
}
