#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
    FILE *fp_taster;
    FILE *fp_timer;
    
    char *buffer;
    char tval1,tval2,tval3,tval4;
    
    // 0 b X X X X \0 -> ukupno 7
    size_t num_of_bytes_taster = 7;
    // hh(2) :(1) mm(2) :(1) ss(2) :(1) ms(3) \0 -> ukupno 13
    size_t num_of_bytes_time = 13;

    int should_print_time = 0;
    int ready_to_print = 1;

    fp_taster = fopen("/dev/timer", "r");
    if(fp_taster == NULL) {
        puts("Problem pri otvaranju /dev/timer");
        return -1;
    }
    
    buffer = (char*)malloc(num_of_bytes_time);
    getline(&buffer, &num_of_bytes_time, fp_taster);
    
    if(fclose(fp_taster)) {
        puts("Problem pri zatvaranju /dev/timer");
        return -1;
    }

    // Ispisivanje trenutnog vremena
    printf("Trenutno vreme: %s\n", buffer);

    free(buffer);

    while(1)
    {
        fp_taster = fopen("/dev/button", "r");
        if(fp_taster == NULL) {
            puts("Problem pri otvaranju /dev/button");
            return -1;
        }

        buffer = (char*)malloc(num_of_bytes_taster);
        getline(&buffer, &num_of_bytes_taster, fp_taster);
        
        if(fclose(fp_taster)) {
            puts("Problem pri zatvaranju /dev/button");
            return -1;
        }

        tval1 = buffer[2] - 48;
        tval2 = buffer[3] - 48;
        tval3 = buffer[4] - 48;
        tval4 = buffer[5] - 48;
        free(buffer);

        sleep(1);

        // Start/pauza
        if (tval1 == 1) {
            if (ready_to_print == 1) {
                should_print_time = 1;
            }
            printf("Taster 1 je pritisnut\n");
            
            fp_taster = fopen("/dev/timer", "w");
            if(fp_taster == NULL) {
                puts("Problem pri otvaranju /dev/timer");
                return -1;
            }

            fputs("toggle", fp_taster);
            
            if(fclose(fp_taster)) {
                puts("Problem pri zatvaranju /dev/timer");
                return -1;
            }
        }
        // dec
        else if (tval2 == 1) {
            if (ready_to_print == 1) {
                should_print_time = 1;
            }
            printf("Taster 2 je pritisnut\n");
            
            fp_taster = fopen("/dev/timer", "w");
            if(fp_taster == NULL) {
                puts("Problem pri otvaranju /dev/timer");
                return -1;
            }

            fputs("dec", fp_taster);
            
            if(fclose(fp_taster)) {
                puts("Problem pri zatvaranju /dev/timer");
                return -1;
            }
        }
        // inc
        else if (tval3 == 1) {
            if (ready_to_print == 1) {
                should_print_time = 1;
            }
            printf("Taster 3 je pritisnut\n");
            
            fp_taster = fopen("/dev/timer", "w");
            if(fp_taster == NULL) {
                puts("Problem pri otvaranju /dev/timer");
                return -1;
            }

            fputs("inc", fp_taster);
            
            if(fclose(fp_taster)) {
                puts("Problem pri zatvaranju /dev/timer");
                return -1;
            }
        }
        // exit
        else if (tval4 == 1) {
            if (ready_to_print == 1) {
                should_print_time = 1;
            }
            printf("Taster 4 je pritisnut\n");
            
            fp_taster = fopen("/dev/timer", "w");
            if(fp_taster == NULL) {
                puts("Problem pri otvaranju /dev/timer");
                return -1;
            }

            fputs("exit", fp_taster);
            
            if(fclose(fp_taster)) {
                puts("Problem pri zatvaranju /dev/timer");
                return -1;
            }

            // Izlaz iz programa
            return 0;
        }
        else {
            ready_to_print = 1;
        }

        if (should_print_time == 1) {
            fp_taster = fopen("/dev/timer", "r");
            if(fp_taster == NULL) {
                puts("Problem pri otvaranju /dev/timer");
                return -1;
            }
            
            buffer = (char*)malloc(num_of_bytes_time);
            getline(&buffer, &num_of_bytes_time, fp_taster);
            
            if(fclose(fp_taster)) {
                puts("Problem pri zatvaranju /dev/timer");
                return -1;
            }

            // Ispisivanje trenutnog vremena
            printf("Trenutno vreme: %s\n", buffer);

            should_print_time = 0;
            ready_to_print = 0;

            free(buffer);
        }
    }
}