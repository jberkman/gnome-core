#include "gmenu.h"


char *homedir(int uid)
{
   FILE *f;
   char s[1024],user[64],home[512],ids[32];
   int id,i,j;
   
   f=fopen("/etc/passwd","r");
   while(fgets(s,1024,f))
     {
	i=0;j=0;
	while (s[i])
	  {
	     user[j]=s[i];
	     if (s[i]==':') 
	       {
		  user[j]=0;
		  j=0;i++;
		  break;
	       }
	     i++;j++;
	  }
	while ((s[i]!=':')&&(s[i])) i++;
	if (s[i]==':') i++;
	while (s[i])
	  {
	     ids[j]=s[i];
	     if (s[i]==':') 
	       {
		  ids[j]=0;
		  j=0;i++;
		  break;
	       }
	     i++;j++;
	  }
	id=atoi(ids);
	while ((s[i]!=':')&&(s[i])) i++;
	if (s[i]==':') i++;
	while ((s[i]!=':')&&(s[i])) i++;
	if (s[i]==':') i++;
	while (s[i])
	  {
	     home[j]=s[i];
	     if (s[i]==':') 
	       {
		  home[j]=0;
		  j=0;i++;
		  break;
	       }
	     i++;j++;
	  }
	if (id==uid)
	  {
	     fclose(f);
	     return strdup(home);
	  }
     }
   return NULL;
}

int isfile(char *s)
{
   struct stat st;
   
   if ((!s)||(!*s)) return 0;
   if (stat(s,&st)<0) return 0;
   if (S_ISREG(st.st_mode)) return 1;
   return 0;
}

int isdir(char *s)
{
   struct stat st;
   
   if ((!s)||(!*s)) return 0;
   if (stat(s,&st)<0) return 0;
   if (S_ISDIR(st.st_mode)) return 1;
   return 0;
}

int filesize(char *s)
{
   struct stat st;
   
   if ((!s)||(!*s)) return 0;
   if (stat(s,&st)<0) return 0;
   return (int)st.st_size;
}

void get_current_dir()
{
/*	getcwd(current_dir,255);*/

}

void set_current_dir(char *s)
{
	chdir(s);
	get_current_dir();
}


