#include <stdio.h>
#include <string.h>

extern int main(int argc, char **argv) {
  FILE *f;
  char buff[100000];
  char *mcp, *link, *spa, *project1, *project2, *project5;

  f = fopen("exec.log", "a");

  mcp = strdup(gets(buff));
  fprintf(stderr, "mcp = [%s]\n", mcp);

  link = strdup(gets(buff));
  fprintf(stderr, "link = [%s]\n", link);

  spa = strdup(gets(buff));
  fprintf(stderr, "spa = [%s]\n", spa);

  project1 = strdup(gets(buff));
  fprintf(stderr, "project1 = [%s]\n", project1);
  project2 = strdup(gets(buff));
  fprintf(stderr, "project2 = [%s]\n", project2);
  project5 = strdup(gets(buff));
  fprintf(stderr, "project5 = [%s]\n", project5);

  fprintf(f, "mcp = [%s]\n", mcp);
  fprintf(f, "link = [%s]\n", link);
  fprintf(f, "spa = [%s]\n", spa);
  fprintf(f, "project1 = [%s]\n", project1);
  fprintf(f, "project2 = [%s]\n", project2);
  fprintf(f, "project5 = [%s]\n", project5);

  fclose(f);

  printf("mcparea.rc=0&");
  printf("mcparea.dc.puttype=&");
  printf("mcparea.dc.window=project1&");
  printf("mcparea.dc.widget=entry2&");
  printf("mcparea.dc.status=PUTG\n");

  printf("demolink.linktext=abcde\n");

  printf("spa.count=0\n");

  printf("project1.vbox1.entry1.value=abcde&");
  printf("project1.vbox1.entry2.value=1234.56\n");

  printf("\n");

  printf("project5.vbox1.calendar1.year=2003&");
  printf("project5.vbox1.calendar1.month=9&");
  printf("project5.vbox1.calendar1.day=3\n");
}
