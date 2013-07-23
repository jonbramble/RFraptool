#include <tinyxml.h>

class Ome 
{
  public:
   Ome(char* data); 
   const char* getimagetime();

  private:
   const char* imagetime; 

};
