#include <Rcpp.h>
using namespace Rcpp;

// [[Rcpp::export]]
List frap_wrapper() {

    # Call frap from here
  
    CharacterVector x = CharacterVector::create( "foo", "bar" )  ;
    NumericVector y   = NumericVector::create( 0.0, 1.0 ) ;
    List z            = List::create( x, y ) ;
    
    return z ;
  
}