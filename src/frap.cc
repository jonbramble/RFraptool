/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */

/*
 * frap.cc
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

#include "frap.h" 

namespace FrapTool 
{

/*-- Constructor Destructors ---------------------------------------------------------------------------------*/
Frap::Frap(std::string pfile, std::string cfile, bool _verbose){
    verbose = _verbose;
    prima = new char[pfile.length()+1];
    closed = new char[cfile.length()+1];
    strcpy(prima,pfile.c_str());
    strcpy(closed,cfile.c_str());

    start_time = 10.0;
    npoints = 250;

    alloc_prima_name = true;
    alloc_closed_name = true;
}

Frap::Frap(){
	start_time = 10.0;
    npoints = 250;
	verbose = false;
    alloc_prima_name = false;
    alloc_closed_name = false;
}

Frap::~Frap(){
    if (sel_made){
        gsl_matrix_free(exp_data);
        gsl_matrix_free(fitting_data);
    }
    if (alloc_prima_name) delete [] prima;
    if (alloc_closed_name) delete [] closed;

}

/*-- Data Functions ----------------------------------------------------------------------------*/
gsl_matrix* Frap::get_exp_data(){
    return exp_data;
}

gsl_matrix* Frap::get_fitting_data(){
    return fitting_data;
}

double Frap::dif_const(){
    return c1/2;
}

double Frap::get_m(){
    return c1;
}

double Frap::get_c(){
    return c0;
}

bool Frap::selected(){
    return s->selmade;
}

void Frap::setprima(std::string pfile){
    prima = new char[pfile.length()+1];
    strcpy(prima,pfile.c_str());
    alloc_prima_name = true;
}

void Frap::setclosed(std::string cfile){
	closed = new char[cfile.length()+1];
	strcpy(closed,cfile.c_str());
    alloc_closed_name = true;
}

/*-- Processing ---------------------------------------------------------------------------------*/
void Frap::setimagenames(std::vector<std::string> ifiles){
    frapimages.clear();
    for(fnameit=ifiles.begin(); fnameit<ifiles.end(); fnameit++){
        Frapimage tmpimage(*fnameit);
        frapimages.push_back(tmpimage);
    }
} 

void Frap::doselection(){
	s = new Selection(prima,closed,frapimages.front().getfilename());
    s->selectline();  //do selection on the closed image
	sel_made = s->selmade;
}

/*void Frap::getfftransforms(){
    cimg_library::CImg<float> tmp;
    cimg_library::CImg<unsigned char> mag;
    //transform must be done centred on the image
    //it must also be scaled to 2^N pixels
    for(cimg_imageit=imagelist.begin(); cimg_imageit<imagelist.end(); cimg_imageit++){
        tmp = cimg_imageit->crop(s.getx1(),s.gety1(),s.getx2(),s.getx2()).resize(256,256).normalize(0,255); // use selection image numbers
        cimg_library::CImgList<float> tmp_fft = tmp.get_FFT();
        mag = ((tmp_fft[0].get_pow(2) + tmp_fft[1].get_pow(2)).sqrt()+1).log().normalize(0,255);
        //transforms.push_back(tmp_img.get_FFT());
    }

    cimg_library::CImgDisplay disp3(tmp,"Spatial Domain",0);
    while (!disp3.is_closed()) {
      disp3.wait();
    }

    cimg_library::CImgDisplay disp2(mag,"Frequency Domain (Log)",0);
    while (!disp2.is_closed()) {
      disp2.wait();
    }

}*/

void Frap::processdata()
{
    //might need to have a reset function here somewhere
    dosort();	// sort by time
    setimagelist();	// put the images in a list
    settimes();

    boost::thread t1(boost::bind( &Frap::setpixlen , this ));
    boost::thread t2(boost::bind( &Frap::removebackground, this ));
    t1.join();
    t2.join();

    getvectors();  // get the data and put it in a matrix
    dofitting(); // do the multid fitting on the gaussian profiles
    linearfit(); // do the linear fit now
    //getfftransforms(); //should do these on centred cropped images

    create_fit_data();
}

void Frap::dosort(){
    std::sort(frapimages.begin(), frapimages.end()); // uses overloaded < operator that compares the seconds from epoch
}

void Frap::get_results(std::vector<result> &results){
    results.clear();
    // could use transform here
    for(frapimage_it=frapimages.begin(); frapimage_it<frapimages.end(); frapimage_it++){
        results.push_back(frapimage_it->r);
    }
}

void Frap::print_data(){
    unsigned int i = 0;
    std::cout <<  "Filename" << "Times" << "A" << "Lambda^2" << "mu" << std::endl;
    for(frapimage_it=frapimages.begin(); frapimage_it<frapimages.end(); frapimage_it++){
        std::cout << frapimage_it->getfilename()<< "\t" << time_s[i] << "\t" << frapimage_it->r.A << "\t" <<frapimage_it->r.lambda_2 << "\t"<< frapimage_it->r.mu << std::endl;
        i++;
    }
    std::cout << "Diffusion Constant " << c1/2 << " μm2/s" << std::endl;
}

void Frap::save_data_file(char* _prefix){
    double lambda_fit;
    unsigned int i = 0;

    char str_summary[80]; //string preparations for filenames
    //char str_fullset[80];
    //char str_linear_fit[80];

    strcpy(str_summary,_prefix);
    //strcpy(str_fullset,_prefix);
    //strcpy(str_linear_fit,_prefix);

    strcat(str_summary,"-summary.csv");
    //strcat(str_fullset,"-data-set.csv");
    //strcat(str_linear_fit,"-linear-fit.csv");

    std::ofstream data_file;

    std::cout << "Writing Datafiles...";

    data_file.open (str_summary);
    if (data_file.is_open())
    {
        data_file << "Filename,Time(s),A,A error,Lambda^2,Lambda error ^2,mu,mu error,b,b error,Fitted Lambda\n";
        for(frapimage_it=frapimages.begin(); frapimage_it<frapimages.end(); frapimage_it++){
            lambda_fit = c0+c1*time_s[i];
            data_file << frapimage_it->getfilename() << "," << frapimage_it->r.time_s <<"," << frapimage_it->r.A <<"," << frapimage_it->r.A_err
                      <<"," << frapimage_it->r.lambda_2 <<"," << frapimage_it->r.lambda_err_2 <<","
                     << frapimage_it->r.mu <<"," << frapimage_it->r.mu_err <<"," << frapimage_it->r.b <<"," << frapimage_it->r.b_err << "," << lambda_fit << std::endl;
            i++;
        }
        data_file << "Diffusion Constant " << c1/2 << " μm2/s" << std::endl;
    }
    data_file.close();

    //data_file.open (str_fullset);
    //data_file << "Writing this to a file.\n";
    //data_file.close();

    //data_file.open (str_linear_fit);
    //data_file << "Writing this to a file.\n";
    //data_file.close();

    std::cout << "...complete" << std::endl;
}


void Frap::setimagelist(){			// limited by disc speed 
    imagelist.clear();
    for(frapimage_it=frapimages.begin(); frapimage_it<frapimages.end(); frapimage_it++){ 
        cimg_library::CImg<float> tmp_image;
        std::string filename = frapimage_it->getfilename();
        char *cstr;
        cstr = new char[filename.length()+1];
        strcpy(cstr,filename.c_str());
        tmp_image.load(cstr);
        imagelist.push_back(tmp_image);
		delete [] cstr;
    }
}

void Frap::settimes(){
    time_s.clear();
    double starttime = frapimages.front().gettime()-start_time; 							//ten second start time
    for(frapimage_it=frapimages.begin(); frapimage_it<frapimages.end(); frapimage_it++){
        if(frapimage_it->r.use_in_fit){
			time_s.push_back(frapimage_it->gettime()-starttime);
		}
        frapimage_it->r.time_s=frapimage_it->gettime()-starttime;
    }
}

void Frap::setpixlen(){
    int binning = 1344/( frapimages.front().getimagewidth());
    int objective = 40;

    switch( binning )
    {
    case 1:
        pixlen = (double)150.0/942.0;       // microns/pixels
        break;
    case 2:
        pixlen = (double)150.0/471.0;
        break;
    case 4:
        pixlen = (double)150.0/235.5;
        break;
    }
}

void Frap::removebackground(){
    std::cout << prima << std::endl;
    cimg_library::CImg<float> primaimg(prima);
    for(cimg_imageit=imagelist.begin(); cimg_imageit<imagelist.end(); cimg_imageit++){
        *cimg_imageit=primaimg-(*cimg_imageit);
    }
}

void Frap::getvectors(){
    if(sel_made){

        float valimage, xk, yk, xstep, ystep;
        int x1,y1, x2, y2;
        int i=0;

        unsigned int size = frapimages.size();

        exp_data = gsl_matrix_alloc (size, npoints);
        fitting_data = gsl_matrix_alloc (size, npoints);

        x1 = s->getx1();
        x2 = s->getx2();
        y1 = s->gety1();
        y2 = s->gety2();

        std::cout << "(" << x1 << "," << y1 << ")"<< "(" << x2 << "," << y2 << ")" << std::endl;

        scaling_factor = (hypot(s->getxsize(),s->getysize())/npoints)*pixlen;
        //std::cout << "scaling factor " << scaling_factor << std::endl;

        if(s->getxsize()==0)
            xstep = 0.0;
        else
            xstep = (float) s->getxsize()/npoints;
        if(s->getysize()==0)
            ystep = 0.0;
        else
            ystep = (float) s->getysize()/npoints;

        for(cimg_imageit=imagelist.begin(); cimg_imageit<imagelist.end(); cimg_imageit++)
        {
            for(uint k=0;k<npoints;k++){
                xk=x1+k*xstep; // calc x and y values
                yk=y1+k*ystep;
                valimage= cimg_imageit->cubic_atXY(xk,yk);   //cubic interpolation
                gsl_matrix_set (exp_data, i, k, valimage);
            }
            i++;
        }
    }
    else {
        std::cout << "no selection made" << std::endl;
    }

	delete s;
}

void Frap::dofitting(){

    lambda_2.clear();
    lambda_err_2.clear();

    double scaled_lambda, scaled_lambda_err;
    unsigned int i = 0;
    int ifilestotal = frapimages.size();

    gsl_matrix *vdata = gsl_matrix_alloc(4,ifilestotal);
    gsl_matrix *verr = gsl_matrix_alloc(4,ifilestotal);

    Fitting::gaussfit(vdata,verr,get_exp_data(),verbose); // fits the data for all images

    for(frapimage_it=frapimages.begin(); frapimage_it<frapimages.end(); frapimage_it++){
        frapimage_it->r.A = gsl_matrix_get(vdata,0,i);
        frapimage_it->r.mu = gsl_matrix_get(vdata,1,i);
        scaled_lambda = gsl_matrix_get(vdata,2,i)*scaling_factor;
        frapimage_it->r.lambda = scaled_lambda;
        frapimage_it->r.lambda_2 = pow(scaled_lambda,2);
        frapimage_it->r.b = gsl_matrix_get(vdata,3,i);

		if(frapimage_it->r.use_in_fit){
        	lambda_2.push_back(pow(scaled_lambda,2));
		}

        frapimage_it->r.A_err = gsl_matrix_get(verr,0,i);
        frapimage_it->r.mu_err = gsl_matrix_get(verr,1,i);
        scaled_lambda_err= gsl_matrix_get(verr,2,i)*scaling_factor;
        frapimage_it->r.lambda_err = scaled_lambda_err;
        frapimage_it->r.lambda_err_2 = pow(scaled_lambda_err,2);
        frapimage_it->r.b_err = gsl_matrix_get(verr,3,i);

        lambda_err_2.push_back(pow(scaled_lambda_err,2));
        i++;
    }

    gsl_matrix_free(vdata);
    gsl_matrix_free(verr);
}

void Frap::linearfit(){

    gsl_vector *fit = gsl_vector_alloc(6);
    int ifilestotal = frapimages.size();

    //int ts = time_s.size();
    //int lms = lambda_2.size();

    Fitting::linearfit(fit, &time_s[0],&lambda_2[0], ifilestotal, verbose); // needs lots of error handling --needs R factor cutoff

    c0 = gsl_vector_get(fit,0);
    c1 = gsl_vector_get(fit,1);
    cov00 = gsl_vector_get(fit,2);
    cov01 = gsl_vector_get(fit,3);
    cov11 = gsl_vector_get(fit,4);
    sumsq = gsl_vector_get(fit,5);

    gsl_vector_free(fit);
}


void Frap::create_fit_data()// create curve data from fits
{
    double Yi, x_scaled, f_A, f_mu, f_lambda_2, f_b;
    unsigned int i = 0;
    for(frapimage_it=frapimages.begin(); frapimage_it<frapimages.end(); frapimage_it++){
        f_A = frapimage_it->r.A;
        f_mu = frapimage_it->r.mu;
        f_lambda_2 = frapimage_it->r.lambda_2;
        f_b = frapimage_it->r.b;

        for(int x=0;x<npoints;x++){
            x_scaled = (x-f_mu)*scaling_factor;
            Yi = f_A*exp((-1*pow(x_scaled,2))/(2*f_lambda_2))+f_b;
            gsl_matrix_set(fitting_data,i,x,Yi);
        }
        i++;
    }
}

}
