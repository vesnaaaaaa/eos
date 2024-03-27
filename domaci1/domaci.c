#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
        FILE *fp_led; // adresa od led fajla
        FILE *fp_taster; // adresa od taster fajla
        FILE *fp_prekidac; // adresa od prekidac fajla
        char *buffer;
        char sval1,sval2,sval3,sval4; //prekidaci
        char tval1,tval2,tval3,tval4; //tasteri
        size_t num_of_bytes = 6;
        int pobeda;

        char* kombinacije[3] = {"0b1101\n",
                                "0b0101\n",
                                "0b0011\n"};

        while(1)
        {
                pobeda = 1;

                //prva kombinacija
                fp_led = fopen("dev/led", "w");
                if(fp_led == NULL)
                {
                        printf("Problem pri otvaranju dev/led\n");
                        return -1;
                }
                sleep(0.5);

                fputs(kombinacije[0], fp_led);
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

                fputs(kombinacije[1], fp_led);
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

                fputs(kombinacije[2], fp_led);
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

                for (int i = 0; i < 3; i++) {
                        fp_taster = fopen("dev/button", "r");
                        if (fp_taster == NULL)
                        {
                                printf("Problem pri otvaranju dev/button\n");
                                return -1;
                        }

                        // Citanje tastera
                        buffer = (char *) malloc(num_of_bytes + 1); // Rezervisemo memoriju za buffer
                        getline(&buffer, &num_of_bytes, fp_taster); // Citamo iz button fajla num_of_bytes 
                                                                // (6) bajtova i smestamo u buffer

                        if (fclose(fp_taster)) {
                                printf("Problem pri zatvaranju dev/button\n");
                                return -1;
                        }

                        tval1 = buffer[2] - '0';
                        free(buffer);

                        // Cekamo pritisak prvog tastera
                        while (tval1 != 1) {
                                fp_taster = fopen("dev/button", "r");
                                if (fp_taster == NULL)
                                {
                                        printf("Problem pri otvaranju dev/button\n");
                                        return -1;
                                }

                                buffer = (char *) malloc(num_of_bytes + 1);
                                getline(&buffer, &num_of_bytes, fp_taster); 

                                if (fclose(fp_taster)) {
                                        printf("Problem pri zatvaranju dev/button\n");
                                        return -1;
                                }

                                tval1 = buffer[2] - '0';
                        }

                        // Citanje prekidaca
                        fp_prekidac = fopen("dev/switch", "r");
                        if (fp_prekidac == NULL)
                        {
                                printf("Problem pri otvaranju dev/switch\n");
                                return -1;
                        }

                        buffer = (char *) malloc(num_of_bytes + 1);
                        getline(&buffer, &num_of_bytes, fp_prekidac); 

                        if (fclose(fp_prekidac)) {
                                printf("Problem pri zatvaranju dev/switch\n");
                                return -1;
                        }
                                
                        sval1 = buffer[2] - '0';
                        sval2 = buffer[3] - '0';
                        sval3 = buffer[4] - '0';
                        sval4 = buffer[5] - '0';
						
						

                        // Poredjenje sa kombinacijama
                        if ((sval1 == kombinacije[i][2]) &&
                            (sval2 == kombinacije[i][3]) &&
                            (sval3 == kombinacije[i][4]) &&
                            (sval4 == kombinacije[i][5]))
                        {
                                // Ako je tacno
							//  pobeda = 1;
                        }
                        else
                        {
                                // Ako nije tacno
                                pobeda = 0;
                                break;
                        }
                }

                // Da li smo pogodili ili ne?
				
				
				
                if (pobeda == 1)
                {
                        // Pobednicka kombinacija

				fp_led = fopen("dev/led", "w");
                if(fp_led == NULL)
                {
                        printf("Problem pri otvaranju dev/led\n");
                        return -1;
                }
                sleep(0.25);

                fputs("0b0000\n", fp_led);
                if (fclose(fp_led))
                {
                        printf("Problem pri zatvaranju dev/led\n");
                        return -1;
				}
				sleep(0.25);
				
				fp_led = fopen("dev/led", "w");
                if(fp_led == NULL)
                {
                        printf("Problem pri otvaranju dev/led\n");
                        return -1;
                }
                sleep(0.25);

                fputs("0b0001\n", fp_led);
                if (fclose(fp_led))
                {
                        printf("Problem pri zatvaranju dev/led\n");
                        return -1;
				}
				sleep (0.25)
				
				
				fp_led = fopen("dev/led", "w");
                if(fp_led == NULL)
                {
                        printf("Problem pri otvaranju dev/led\n");
                        return -1;
                }
                sleep(0.25);

                fputs("0b0010\n", fp_led);
                if (fclose(fp_led))
                {
                        printf("Problem pri zatvaranju dev/led\n");
                        return -1;
				}
				sleep(0.25);fp_led = fopen("dev/led", "w");
                if(fp_led == NULL)
                {
                        printf("Problem pri otvaranju dev/led\n");
                        return -1;
                }
                sleep(0.25);

                fputs("0b0100\n", fp_led);
                if (fclose(fp_led))
                {
                        printf("Problem pri zatvaranju dev/led\n");
                        return -1;
				}
				sleep(0.25);
				fp_led = fopen("dev/led", "w");
                if(fp_led == NULL)
                {
                        printf("Problem pri otvaranju dev/led\n");
                        return -1;
                }
                sleep(0.25);

                fputs("0b1000\n", fp_led);
                if (fclose(fp_led))
                {
                        printf("Problem pri zatvaranju dev/led\n");
                        return -1;
				}
				sleep(0.25);
				
				fp_led = fopen("dev/led", "w");
                if(fp_led == NULL)
                {
                        printf("Problem pri otvaranju dev/led\n");
                        return -1;
                }
                sleep(0.25);

                fputs("0b0100\n", fp_led);
                if (fclose(fp_led))
                {
                        printf("Problem pri zatvaranju dev/led\n");
                        return -1;
				}
				sleep(0.25);
				
				fp_led = fopen("dev/led", "w");
                if(fp_led == NULL)
                {
                        printf("Problem pri otvaranju dev/led\n");
                        return -1;
                }
                sleep(0.25);

                fputs("0b0010\n", fp_led);
                if (fclose(fp_led))
                {
                        printf("Problem pri zatvaranju dev/led\n");
                        return -1;
				}
				sleep(0.25);
				fp_led = fopen("dev/led", "w");
                if(fp_led == NULL)
                {
                        printf("Problem pri otvaranju dev/led\n");
                        return -1;
                }
                sleep(0.25);

                fputs("0b0001\n", fp_led);
                if (fclose(fp_led))
                {
                        printf("Problem pri zatvaranju dev/led\n");
                        return -1;
				}
				sleep(0.25);
				
                }
                else
                {
                        // Gubitnicka kombinacija

                fp_led = fopen("dev/led", "w");
                if(fp_led == NULL)
                {
                        printf("Problem pri otvaranju dev/led\n");
                        return -1;
                }
                sleep(0.33);

                fputs("0b0000\n", fp_led);
                if (fclose(fp_led))
                {
                        printf("Problem pri zatvaranju dev/led\n");
                        return -1;
				}
				sleep(0.33);
				
				fp_led = fopen("dev/led", "w");
                if(fp_led == NULL)
                {
                        printf("Problem pri otvaranju dev/led\n");
                        return -1;
                }
                sleep(0.25);

                fputs("0b1111\n", fp_led);
                if (fclose(fp_led))
                {
                        printf("Problem pri zatvaranju dev/led\n");
                        return -1;
				}
				sleep(0.33);
				
				fp_led = fopen("dev/led", "w");
                if(fp_led == NULL)
                {
                        printf("Problem pri otvaranju dev/led\n");
                        return -1;
                }
                sleep(0.33);

                fputs("0b0000\n", fp_led);
                if (fclose(fp_led))
                {
                        printf("Problem pri zatvaranju dev/led\n");
                        return -1;
				}
				sleep(0.33);
				
				fp_led = fopen("dev/led", "w");
                if(fp_led == NULL)
                {
                        printf("Problem pri otvaranju dev/led\n");
                        return -1;
                }
                sleep(0.33);

                fputs("0b1111\n", fp_led);
                if (fclose(fp_led))
                {
                        printf("Problem pri zatvaranju dev/led\n");
                        return -1;
				}
				sleep(0.33);
				
				fp_led = fopen("dev/led", "w");
                if(fp_led == NULL)
                {
                        printf("Problem pri otvaranju dev/led\n");
                        return -1;
                }
                sleep(0.33);

                fputs("0b0000\n", fp_led);
                if (fclose(fp_led))
                {
                        printf("Problem pri zatvaranju dev/led\n");
                        return -1;
				}
				sleep(0.33);
				
				fp_led = fopen("dev/led", "w");
                if(fp_led == NULL)
                {
                        printf("Problem pri otvaranju dev/led\n");
                        return -1;
                }
                sleep(0.33);

                fputs("0b1111\n", fp_led);
                if (fclose(fp_led))
                {
                        printf("Problem pri zatvaranju dev/led\n");
                        return -1;
				}
				sleep(0.33);
				
						
				//pritisak poslednjeg tastera 
						
						 while (tval4 != 1) {
                                fp_taster = fopen("dev/button", "r");
                                if (fp_taster == NULL)
                                {
                                        printf("Problem pri otvaranju dev/button\n");
                                        return -1;
                                }

                                buffer = (char *) malloc(num_of_bytes + 1);
                                getline(&buffer, &num_of_bytes, fp_taster); 

                                if (fclose(fp_taster)) {
                                        printf("Problem pri zatvaranju dev/button\n");
                                        return -1;
                                }

                                tval4 = buffer[5] - '0';
						 }
						 
				 // sekvenca koja treba da se pokazuje kada zelimo da izadjemo iz programa
				 
				 
				if (pobeda == 1){   
				
				continue;
				
				}
				
				else {
					
					
				fp_led = fopen("dev/led", "w");
                if(fp_led == NULL)
                {
                        printf("Problem pri otvaranju dev/led\n");
                        return -1;
                }
                sleep(0.5);

                fputs("0b1010\n", fp_led);
                if (fclose(fp_led))
                {
                        printf("Problem pri zatvaranju dev/led\n");
                        return -1;
				}
				sleep(0.5);
				
				fp_led = fopen("dev/led", "w");
                if(fp_led == NULL)
                {
                        printf("Problem pri otvaranju dev/led\n");
                        return -1;
                }
                sleep(0.5);

                fputs("0b0101\n", fp_led);
                if (fclose(fp_led))
                {
                        printf("Problem pri zatvaranju dev/led\n");
                        return -1;
				}
				sleep(0.55);
				fp_led = fopen("dev/led", "w");
                if(fp_led == NULL)
                {
                        printf("Problem pri otvaranju dev/led\n");
                        return -1;
                }
                sleep(0.5);

                fputs("0b1010\n", fp_led);
                if (fclose(fp_led))
                {
                        printf("Problem pri zatvaranju dev/led\n");
                        return -1;
				}
				sleep(0.5);
				fp_led = fopen("dev/led", "w");
                if(fp_led == NULL)
                {
                        printf("Problem pri otvaranju dev/led\n");
                        return -1;
                }
                sleep(0.5);

                fputs("0b0101\n", fp_led);
                if (fclose(fp_led))
                {
                        printf("Problem pri zatvaranju dev/led\n");
                        return -1;
				}
				sleep(0.5);
				
						
						
                }

                sleep(5);
        }
}