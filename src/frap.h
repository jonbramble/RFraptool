/*
 * frap.h
 * Copyright (C) Jonathan Bramble 2011
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

#ifndef FRAP_H
#define FRAP_H

//#define cimg_use_tif 1

#include "CImg.h"
#include "unistd.h"
#include "selection.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>

#include <ctime>
#include <cstring>

#include "frapimage.h"
#include "fitting.h"

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>

namespace FrapTool {

class Frap {
	public: 
    Frap(std::string pfile, std::string cfile, bool verbose);
	  Frap();
		~Frap(); //<must allocate mem to data before calling free
		
		void processdata();
		void doselection();

		gsl_matrix* get_exp_data();
		gsl_matrix* get_fitting_data();
		void setimagenames(std::vector<std::string> ifiles);
		void setprima(std::string prima);
		void setclosed(std::string closed);

		void save_data_file(char* _prefix);
		void print_data();
    double dif_const();
    double get_c();
    double get_m();
    void get_results(std::vector<result> &results);

    bool selected();

	private:
    char* prima;
    char* closed;
		bool verbose;
    bool alloc_prima_name;      // see if file names are allocated
    bool alloc_closed_name;

		int npoints;			 // number of points to use in selection interpolation
		double pixlen;			 //<pixel scaling factor - from microscope
		double scaling_factor; 	 //<pixel scaling factor - from selection length
    double start_time;      // start time should be editable
    Selection *s;
	  bool sel_made;

		gsl_matrix *exp_data;		//<for output data
		gsl_matrix *fitting_data;	//<for output data

		cimg_library::CImg<unsigned char> *visu;	//<graph plot

		void dosort();			//separate sub routines to merge later
		
		void setimagelist();
		void settimes();
		void removebackground();
		void setpixlen();
		void getvectors();
		void getfftransforms();
		void dofitting();
		void linearfit();
		void create_fit_data();

		std::vector<Frapimage> frapimages; //<this vector contains all the information about each file
		std::vector<Frapimage>::iterator frapimage_it;

		std::vector<double> time_s; 	//<time data in seconds from the start point of experiment
		std::vector<double> lambda_2; 	//<lambda data
		std::vector<double> lambda_err_2; 	//<lambda data

		double c0, c1, cov00, cov01, cov11, sumsq;  //<linear fit parameters
		
		cimg_library::CImgList<float> imagelist;	//<this is a list of the image data	
		cimg_library::CImgList<float> transforms;	//<this is a list of the fourier transformed data

		std::vector<std::string>::iterator fnameit;	//iterators
		cimg_library::CImgList<float>::iterator cimg_imageit;
};

} // FrapTool

#endif
