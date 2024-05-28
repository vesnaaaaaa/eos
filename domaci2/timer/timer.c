#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
        FILE *fp_taster;
        
        char *buffer;
        char tval1,tval2,tval3,tval4;
        
        size_t num_of_bytes = 6;

        // TODO 
        // Ispisati trenutno vreme?

        while(1)
        {
            fp_taster = fopen("/dev/button", "r");
            if(fp_taster == NULL) {
                puts("Problem pri otvaranju /dev/button");
                return -1;
            }
            buffer = (char*)malloc(num_of_bytes+1);
            getline(&buffer, &num_of_bytes, fp_taster);
            
            if(fclose(fp_taster)) {
                puts("Problem pri zatvaranju /dev/button");
                return -1;
            }

            tval1 = buffer[2] - 48;
            tval2 = buffer[3] - 48;
            tval3 = buffer[4] - 48;
            tval4 = buffer[5] - 48;
            free(buffer);

            printf("Vrednosti tastera: %d %d %d %d\n", tval1,tval2,tval3,tval4);
            sleep(1);

            if (tval1 == 1) {
                // Start/pauza
                printf("Taster 1 je pritisnut\n");
            }
            if (tval2 == 1) {
                // dec
                printf("Taster 2 je pritisnut\n");
            }
            if (tval3 == 1) {
                // inc
                printf("Taster 3 je pritisnut\n");
            }
            if (tval4 == 1) {
                // exit
                printf("Taster 4 je pritisnut\n");
            }
        }
}