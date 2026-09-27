#include "../src/Timeline.hpp"
#include <cassert>
int main() {
 Timeline t;
 auto& a=t.lanes[0];
 a.input(true,0); a.input(true,0.1);
 assert(a.count==1 && a.holds.size()==1);
 a.input(false,0.25);
 assert(a.holds.front().end-a.holds.front().start==0.25);
 a.prune(1.0,2); assert(a.presses.empty() && a.holds.size()==1);
 a.prune(2.26,2); assert(a.holds.empty());
 a.input(true,3); a.prune(100,2); assert(a.holds.size()==1 && a.down);
 t.now=100; t.release(); assert(!a.down && !a.holds.back().active);
 t.lanes[1].input(true,100); assert(t.lanes[1].count==1 && a.count==2);
 t.reset(); assert(t.now==0 && t.lanes[0].count==0 && t.lanes[1].holds.empty());
 for(int i=0;i<3000;i++) { t.lanes[0].input(true,0); t.lanes[0].input(false,0); }
 assert(t.lanes[0].holds.size()==2048);
}
