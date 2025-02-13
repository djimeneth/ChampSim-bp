/*
 * This program takes an ELF executable and list of symbols generated from the printsymbols.cc code snippet and makes a full disassembly of the program including DLLs
 */
#include <stdio.h>
#include <list>
#include <map>
#include <string>
#include <string.h>
#include <assert.h>

using namespace std;

list<string> files;

map<string, list<string> > dis;
map<string, unsigned long int> base_address;

int main (int argc, char **argv) {
	char s[1000];

	if (argc != 3) {
		fprintf (stderr, "Usage: %s <executable> <symbol output>\n", argv[0]);
		return 1;
	}

	// get the list of DLLs for this executable

	sprintf (s, "ldd %s", argv[1]);
	FILE *f = popen (s, "r");
	for (;;) {
		fgets (s, 1000, f);
		if (feof (f)) break;
		char *p = strchr (s, '/');
		if (p) {
			char *r = strrchr (s, ' ');
			if (r) {
				*r = 0;
				files.push_back (p);
			}
		}
	}
	pclose (f);
	files.push_back (argv[1]);
	for (auto p=files.begin(); p!=files.end(); p++) {
		sprintf (s, "objdump -d %s", (*p).c_str());
		f = popen (s, "r");
		for (;;) {
			fgets (s, 1000, f);
			if (feof (f)) break;
			for (int i=0; s[i]; i++) if (s[i] == '\n') s[i] = 0;
			dis[*p].push_back (s);
		}
		pclose (f);
	}
	// now put together a disassembly of every procedure that begins with a symbol we got from the program
	f = fopen (argv[2], "r");
	for (;;) {
		fgets (s, 1000, f);
		if (feof (f)) break;
		// Library: /lib/x86_64-linux-gnu/libomp.so.5 Base Address: 7fffe21df000
		if (strstr (s, "Library: ")) {
			char t[1000];
			unsigned long int x;
			int k = sscanf (s, "Library: %s Base Address: %lx", t, &x);
			assert (k == 2);
			base_address[t] = x;
		}
	}
	fclose (f);
	for (auto p=files.begin(); p!=files.end(); p++) {
		unsigned long int x = base_address[*p];
		printf ("base address for %s is %lx\n", (*p).c_str(), x);
		list<string> & r = dis[*p];
		for (auto q=r.begin(); q!=r.end(); q++) {
			// if this string begins with spaces then hex digits then a colon, add the base address for it
			const char *z = (*q).c_str();
			if (z[0]) if (!strstr (z, "Disassembly")) {
				while (*z && *z == ' ') z++;
				unsigned long int y;
				int k = sscanf (z, "%lx:", &y);
				if (k == 1) {
					printf ("%lx: ", x + y);
				}
			}
			printf ("%s\n", z);
		}
	}
	return 0;
}
