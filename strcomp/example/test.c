#include "strcomp.h"

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		printf("Incorrect number of arguments!\n");
		
		exit(0);
	}
	
	printf("Longest Common Substring:\t\t%d\n", strcomp_lcs(argv[1], argv[2]));
	printf("Longest Common Substring Normalized:\t%f\n\n", strcomp_lcs_normalized(argv[1], argv[2]));
	printf("Edit Distance:\t\t\t\t%d\n", strcomp_levensthein(argv[1], argv[2]));
	printf("Edit Distance Normalized:\t\t%f\n", strcomp_levensthein_normalized(argv[1], argv[2]));
	
	return 0;
}