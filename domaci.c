#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


int main()
{
        FILE *fp_led; // adresa od led fajla
        FILE *fp_taster; // adresa od taster fajla
        char *str;
        char sva11,sva12,sva13,sva14; //prekidaci
        char tva11,tva12,tva13,tva14; //tasteri
        size_t num_of_bytes = 6;

        while(1)
        {
                //prva kombinacija

                fp_led = fopen("dev/led", "w");
                if(fp_led == NULL)
                {
                        printf("Problem pri otvaranju dev/led\n");
                        return -1;
                }
                sleep(0.5);

                fputs("0b1101\n", fp_led);
                if (fclose(fp_led))
                {
                        printf("Problem pri zatvaranju dev/led\n");
                        return -1;
                }
                sleep(0.5);

                        //druga kombinacija

                fp_led = fopen("dev/led", "w");
                if(fp_led ==NULL)
                {
                        printf("Problem pri otvaranju dev/led\n");
                        return -1;
                }

                fputs("0b0101\n", fp_led);
                if(fclose(fp_led))
                {
                        printf ("Problem pri zatvaranju dev/led\n");
                        return -1;
                }

                		       //treca kombinacija

                fp_led = fopen("/dev/led", "w");
                if (fp_led ==NULL)
                {
                        printf("Problem pri otvaranju dev/led\n");
                        return -1;
                }

                fputs("0b0010\n", fp_led);
                if(fclose(fp_led))
                {
                        printf("Problem pri zatvaranju dev/led\n");
                        return -1;
                }

                sleep(0.5);


                        //gasenje dioda
                fp_led = fopen("dev/led", "w");
                if (fp_led == NULL)
                {
                        printf("Problem pri otvaranju  dev/led\n");
                        return -1;
                }

                fputs("0b0000\n", fp_led);
                if(fclose(fp_led))
                {
                        printf("Problem pri zatvaranju dev/led\n");
                        return -1;
                }
                sleep(0.5);

                fp_taster = fopen("dev/button", "r");
                if (fp_taster == NULL)
                {
                        printf("Problem pri otvaranju  dev/button\n");
                        return -1;
                }

                char* buffer = (char *) malloc(num_of_bytes + 1); // Rezervisemo memoriju za buffer
                getline(&buffer, &num_of_bytes, fp_taster); // Citamo iz button fajla num_of_bytes 
                                                            // (6) bajtova i smestamo u buffer 
        }
}