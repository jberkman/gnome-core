/* little test main() to see how we're doing */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data.h"
#include "html.h"
#include "parse.h"
#include "utils.h"
#include "version.h"

int
main(int argc, char **argv)
{
	FILE *f;
	char line[250];

	char *requested_nodename=NULL;
	int aptr;
	int result;
	int foundit=0;

	NODE *node;
	
	if (!be_quiet)
		printf("info2html Version %s\n",INFO2HTML_VERSION);
	
	/* simplistic command line parsing for now */
	aptr = argc;
	while (aptr > 2) {
		if (!strcmp(argv[argc-aptr+1], "-a")) {
			char *s, *t;
			int  len;

			requested_nodename = strdup(argv[argc-aptr+2]);
			/* strip off quotes */
			for (s=requested_nodename; *s == '\"'; ) {
				len = strlen( s );
				memmove(s, s+1, len);
			}

			t = s + strlen(s) - 1;
			while (*t == '\"')
				t--;

			*(t+1) = '\0';
				
			fprintf(stderr, "outputting node %s\n",
				requested_nodename);
			aptr -= 2;
		} else if (!strcmp(argv[argc-aptr+1], "-b")) {
			BaseFilename = strdup(argv[argc-aptr+2]);
			fprintf(stderr, "outputting basefile %s\n",
			        BaseFilename);
			aptr -= 2;
		}
	}

	if (aptr == 1) {
		f = stdin;
		strcpy(work_filename, "STDIN");
	} else {
		if ((f=fopen(argv[argc-aptr+1], "r"))==NULL) {
			fprintf(stderr, "File %s not found.\n",argv[1]);
			exit(1);
		}
		strcpy(work_filename, argv[1]);
	}

	work_line_number = 0;


	/* scan for start of real data */
	for (;1;) {
		fgets(line,250,f);
		if (feof(f))
		{
			fprintf(stderr,"Info file had no contents\n");
			exit(1);
		}
		
		work_line_number++;
		if (*line == INFO_COOKIE)
			break;
		
	}

	/* hack, just send to stdout for now */
	fprintf(stdout, "<BODY><HTML>\n");
	
	/* big loop to identify sections of info files */
	/* NEW PLAN - format on the fly */
	/* No need to store all nodes, etc since we let web server */
	/* handle resolving tags!                                  */
	for (;1 || !foundit || !requested_nodename;) {
		fgets(line,250,f);
		if (feof(f))
			break;
		
		work_line_number++;
		
		/* found a node definition line */
		if (!strncmp(line, "File:", 5)) {
			node = alloc_node();
			result=read_node( f, line, node );
			if ( result == READ_ERR ) {
				fprintf(stderr, "Error reading the node "
					"contents\n");
				fprintf(stderr, "line was |%s|\n",line);
				continue;
			}
			
			/* see if this is the requested node name */
			if (requested_nodename && 
			    strcmp(requested_nodename, node->nodename)) {
				fprintf(stderr, "skipping ->%s<-\n",
					node->nodename);

				continue;
			}

			foundit = 1;
			strcpy(work_node,node->nodename);
			
#ifdef DEBUG
			printf("NEW NODE\n");
			printf("\tFile:|%s|\n\tNode:|%s|\n\tNext:|%s|\n",
			       node->filename, node->nodename,node->next);
			printf("\tPrev:|%s|\n\tUp:|%s|\n\n", 
			       node->prev, node->up);
			printf("-------------------------------------------"
			       "-----------\n");
#endif
			/* now lets make some html */
			dump_html_for_node( node );
			
			if (node) {
				if ( node->contents )
					free(node->contents);
				
				free(node);
			}
		}
		else
			continue;
	}

	if (!foundit && requested_nodename)
		fprintf(stdout, "Requested node <b>%s</b> not found\n",
			requested_nodename);

	fprintf(stdout, "</BODY></HTML>\n");
	return 0;
}
