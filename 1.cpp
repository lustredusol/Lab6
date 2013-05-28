#include <iostream>
#include <queue>
#include <vector>
#include <list>
#include <string>
#include <algorithm>
#include <set>

using namespace std;

static vector<string> patterns;
static string input;
static int skippenalty;

struct acnode {
      acnode() : failure(NULL), gotofn(256) {}
      struct acnode *failure;
      vector<struct acnode *> gotofn;
      list<int> outputs; // index into patterns global
};

void
add_string_to_trie(acnode *root, const string &s, int sid)
{
   for (string::const_iterator p = s.begin(); p != s.end(); ++p) {
      if (!root->gotofn[*p])
     root->gotofn[*p] = new acnode;
      root = root->gotofn[*p];
   }
   root->outputs.push_back(sid);
}

void
init_tree(acnode *root)
{
   queue<acnode *> q;
   unsigned char c = 0;
   do {
      if (acnode *u = root->gotofn[c]) {
         u->failure = root;
         q.push(u);
      } else
         root->gotofn[c] = root;
   } while (++c);
   while (!q.empty()) {
      acnode *r = q.front();
      q.pop();

      do {
         acnode *u, *v;
         if (!(u = r->gotofn[c]))
            continue;
         q.push(u);
         v = r->failure;
         while (!v->gotofn[c])
            v = v->failure;
         u->failure = v->gotofn[c];
         u->outputs.splice(u->outputs.begin(), v->gotofn[c]->outputs);
      } while (++c);
   }
}

struct match { int begin, end, sid; };

void
ahocorasick(const acnode *state, list<match> &out, const string &str)
{
   int i = 1;
   for (string::const_iterator p = str.begin(); p != str.end(); ++p, ++i) {
      while (!state->gotofn[*p])
         state = state->failure;
      state = state->gotofn[*p];
      for (list<int>::const_iterator q = state->outputs.begin();
           q != state->outputs.end(); ++q) {
         struct match m = { i - patterns[*q].size(), i, *q };
         out.push_back(m);
      }
   }
}

////////////////////////////////////////////////////////////////////////
bool operator<(const match& m1, const match& m2)
{
   return m1.begin < m2.begin
      || (m1.begin == m2.end && m1.end < m2.end);
}

struct dnode {
      int usedchars;
      vector<bool> usedpatterns;
      int last;
};

bool operator<(const dnode& a, const dnode& b) {
   return a.usedchars > b.usedchars
      || (a.usedchars == b.usedchars && a.usedpatterns < b.usedpatterns);
}
bool operator==(const dnode& a, const dnode& b) {
   return a.usedchars == b.usedchars
      && a.usedpatterns == b.usedpatterns;
}

typedef priority_queue<pair<int, dnode>,
               vector<pair<int, dnode> >,
               greater<pair<int, dnode> > > mypq;

void
dijkstra(const vector<match> &matches)
{
   typedef vector<match>::const_iterator mIt;
   vector<bool> used(patterns.size(), false);
   dnode initial = { 0, used, -1 };
   mypq q;

   set<dnode> last;
   dnode d;

   q.push(make_pair(0, initial));
   while (!q.empty()) {
      int cost = q.top().first;
      d = q.top().second;
      q.pop();

      if (last.end() != last.find(d)) // we've been here before
         continue;

      last.insert(d);
      if (d.usedchars >= input.size()) {
         break; // found optimum
      }

      match m = { d.usedchars, 0, 0 };      
      mIt mp = lower_bound(matches.begin(), matches.end(), m);

      if (matches.end() == mp) {
         // no more matches, skip the remaining string
         dnode nextd = d;
         d.usedchars = input.size();
         int skip = nextd.usedchars - d.usedchars;
         nextd.last = -skip;

         q.push(make_pair(cost + skip * skippenalty, nextd));
         continue;
      }

      // keep track of where the shortest match ended; we don't need to
      // skip more than this.
      int skipmax = (mp->begin == d.usedchars) ? mp->end : mp->begin + 1;
      while (mp != matches.end() && mp->begin == d.usedchars) {
         dnode nextd = d;
         nextd.usedchars = mp->end;
         int extra = nextd.usedpatterns[mp->sid] ? 0 : 1; // extra pattern
         int nextcost = cost + extra;
         nextd.usedpatterns[mp->sid] = true;
         nextd.last = mp->sid * 2 + extra; // encode used pattern
         q.push(make_pair(nextcost, nextd));
         ++mp;
      }

      if (mp == matches.end() || skipmax <= mp->begin)
         continue;

      // skip
      dnode nextd = d;
      nextd.usedchars = mp->begin;
      int skip = nextd.usedchars - d.usedchars;
      nextd.last = -skip;
      q.push(make_pair(cost + skip * skippenalty, nextd));
   }

   // unwind
   string answer;
   while (d.usedchars > 0) {
      if (0 > d.last) {
         answer = string(-d.last, '*') + answer;
         d.usedchars += d.last;
      } else {
         answer = "[" + patterns[d.last / 2] + "]" + answer;
         d.usedpatterns[d.last / 2] = !(d.last % 2);
         d.usedchars -= patterns[d.last / 2].length();
      }

      set<dnode>::const_iterator lp = last.find(d);
      if (last.end() == lp) return; // should not happen
      d.last = lp->last;
   }
   cout << answer;
}

int
main()
{
   int n;
   cin >> n; // read n patterns
   patterns.reserve(n);
   acnode root;
   for (int i = 0; i < n; ++i) {
      string s;
      cin >> s;
      patterns.push_back(s);
      add_string_to_trie(&root, s, i);
   }
   init_tree(&root);

   getline(cin, input); // eat the rest of the first line
   getline(cin, input);
   cerr << "got input: " << input << endl;
   list<match> matches;
   ahocorasick(&root, matches, input);

   vector<match> vmatches(matches.begin(), matches.end());
   sort(vmatches.begin(), vmatches.end());
   skippenalty = 1 + patterns.size();

   dijkstra(vmatches);
   return 0;
}