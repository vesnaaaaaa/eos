#include <stdio.h>
#include <stdlib.h>
#include <unistd>


int main()
{
        FILE* fp;
        char *str;
        char sva11,sva12,sva13,sva14; //prekidaci
        char tva11,tva12,tva13,tva14; //tasteri

        while(1)
        {
                //prva kombinacija

                fp = fopen("dev/led", "w");
                if(fp == NULL)
                {
                        printf("Problem pri otvaranju dev/led\n");
                        return -1;
                }
                sleep(0.5);

                fputs("0b1101\n", fp);
                if (fclose(fp))
                {
                        printf("Problem pri zatvaranju dev/led\n");
                        return -1;
                }
                sleep(0.5);

                        //druga kombinacija

                fp = fopen("dev/led", "w");
                if(fp ==NULL)
                {
                        printf("Problem pri otvaranju dev/led\n");
                        return -1;
                }

                fputs("0b0101\n", fp);
                if(fclose(fp))
                {
                        printf ("Problem pri zatvaranju dev/led\n");
                        return -1;
                }

                		       //treca kombinacija

                fp = fopen("/dev/led", "w");
                if (fp ==NULL)
                {
                        printf("Problem pri otvaranju dev/led\n");
                        return -1;
                }

                fputs("0b0010\n", fp);
                if(fclose(fp))
                {
                        printf("Problem pri zatvaranju dev/led\n");
                        return -1;
                }

                sleep(0.5);


                        //gasenje dioda
                fp = fopen("dev/led", "w");
                if (fp == NULL)
                {
                        printf("Problem pri otvaranju  dev/led\n");
                        return -1;
                }

                fputs("0b0000\n", fp);
                if(fclose(fp))
                {
                        printf("Problem pri zatvaranju dev/led\n");
                        return -1;
                }

                sleep(0.5);
        }
}