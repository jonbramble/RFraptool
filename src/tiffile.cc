/*
 * tiffile.cc
 * Copyright (C) Jonathan Bramble 2013
 * 
frap-tool is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * frap-tool is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "tiffile.h" 

namespace FrapTool 
{

int Tiffile::getimagewidth(){
	return imagewidth;
}

int Tiffile::getimageheight(){
	return imageheight;
}	

tm Tiffile::gettm(){
	return imagetime;
}

std::string Tiffile::getfilename(){
	return filename;
}

Tiffile::Tiffile(std::string _filename){
	filename = _filename;

	std::cout << "Processing Image File...";
        std::cout << ".." << filename << "...";		//output file information
	
	char *cstr;
	const char *to_parse;

	cstr = new char [filename.size()+1];
    	strcpy (cstr, filename.c_str());
	
        TIFFErrorHandler handler;
	handler = TIFFSetWarningHandler(NULL);	//implement warning handler to supress ALL errors due to image pro express
	
	TIFF* tif = TIFFOpen(cstr,"r");  //should check for file here
	TIFFSetWarningHandler(handler);
	
	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &imagewidth);  //get image scales
	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &imageheight);

	TIFFGetField(tif, TIFFTAG_IMAGEDESCRIPTION, &imagedescription);

	Ome *omeparser = new Ome(imagedescription);
	to_parse = omeparser->getimagetime();
	
	strptime(to_parse, "%Y-%m-%dT%T", &imagetime);
	char* mytime = asctime( &imagetime );

	//std::cout << mytime << std::endl;
  
  imagetime.tm_isdst = 0;   // ignors daylight saving settings - otherwise we get an hour shifted
	time_t result = mktime(&imagetime);

	l_seconds = long(result);
	//d_seconds = (double)(long(result));

	TIFFClose(tif);
	delete [] cstr;
	delete omeparser;
	
	std::cout << "...complete" << std::endl;
}

}

	/*char* datetime[4];
	TIFFGetField(tif, TIFFTAG_DATETIME, &datetime); 

	std::cout << *datetime;
	
	char* pch[4];			// does the memory need to be allocated?
	char delims[] = " ";
	char *date,*fulltime;
	  
	//issue with image pro express means datetime is trimmed by tifflib because 27 bytes long
	pch[0] = strtok(*datetime, delims);//chopping into sections with space delimiters recovers the data
	pch[1] = strtok(NULL, delims);
	
	date = pch[0];
	fulltime = pch[1];

	//shorttime = strtok(fulltime, "."); // get and hence remove millisec portion of datetime
	//ms = atoi(strtok(NULL, ".")); // convert to separate integer
	ms = 0;

	char to_parse[30];
	strcpy(to_parse,date);
	strcat(to_parse," ");
	strcat(to_parse,fulltime);
	
	strptime (to_parse, "%m/%d/%Y %H:%M:%S.", &imagetime); //use ctime function to form date time structure

*/
