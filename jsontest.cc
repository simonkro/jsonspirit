#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <time.h>
#include "../json/json.h"

using namespace tbd;
using namespace std;

int count = 0;

string rand_str() {
  std::string s; int l = rand() % 40;
  for (int i = 0; i < l; i++) s += ('a' + (rand() % 26));
  return s;
}

void rand_json(json& value) {
  ++count;
  switch(rand() % 12) {
    case 0: case 1: value = rand() % 10000; break;
    case 2: case 3: value = rand() / 100.0; break;
    case 4: case 5: value = rand_str(); break;
    case 6: case 7: value = ((rand() % 2) == 0); break;
    case 8: case 9:  value = json::null; break;
    case 10: {
      value = json_obj();
      int l = rand() % 13;
      for (int i = 0; i < l; i++) {
        rand_json(value.add(rand_str(), json::null));
      }
    } break;
    case 11: {
      value = json_ary();
      int l = rand() % 13;
      for (int i = 0; i < l; i++) {
        rand_json(value.add(json::null));
      }
    }
  }
}

int main() {
  srand(0);

  json value1 = json(json_ary());

  clock_t t0 = clock();
  for(int i = 0; i < 200; i++) rand_json(value1[i]);

  clock_t t1 = clock();
  printf("elements: %i\n", count);
  printf("create: %gs\n", (t1-t0)/(double)CLOCKS_PER_SEC);

  ofstream ofs = ofstream("test.json");
  ofs << value1 << endl;
  ofs.close();

  clock_t t2 = clock();
  printf("write: %gs\n", (t2-t1)/(double)CLOCKS_PER_SEC);

  json value2;
  ifstream ifs = ifstream("test.json");
  ifs >> value2;
  ifs.close();

  clock_t t3 = clock();
  printf("read: %gs\n", (t3-t2)/(double)CLOCKS_PER_SEC);

  cout << ((value1 == value2) ? "success" : "failure") << endl;

  clock_t t4 = clock();
  printf("compare: %gs\n", (t4-t3)/(double)CLOCKS_PER_SEC);

  return 0;
}
