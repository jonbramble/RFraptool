/*
 * ome.cc
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
 
#include "ome.h"

Ome::Ome(char* data){
 TiXmlDocument doc;   //create a tinyxml doc
 doc.Parse(data, 0, TIXML_ENCODING_UTF8);  // parse the string
 TiXmlNode *image;  // create node and element
 TiXmlElement *date;
 
 TiXmlElement *root = doc.FirstChildElement("OME");
 if(root == NULL){
    // test for first element
    std::cerr << "Failed to load file: No root element." << std::endl;
    doc.Clear();
 }

 image = root->FirstChild("Image");
 date = image->FirstChildElement("AcquisitionDate");
 imagetime = date->GetText(); // Put the datetime in char array

 doc.Clear();
  
}

const char* Ome::getimagetime(){
 return imagetime;      // getter for imagetime
}
