/**
* A list of classes Root does not understand by default and have to
* incorporated into a dictionary.
*/
#include <vector>
#include <map>
#include <string>
#ifdef __MAKECINT__
#pragma extra_include "vector";
#pragma extra_include "map";
//#pragma link C++ class std::vector<bool>+;
//#pragma link C++ class std::vector<short>+;
#pragma link C++ class std::vector<int>+;
//#pragma link C++ class std::vector<long>+;
//#pragma link C++ class std::vector<float>+;
#pragma link C++ class std::vector<std::vector<int> >+;
#pragma link C++ class std::vector<std::vector<double> >+;
#pragma link C++ class std::vector<std::vector<string> >+;
#pragma link C++ class std::map<std::string,bool>+;
#pragma link C++ class std::pair<std::string,bool>+;
#endif
