//
//  main.c
//  Threads
//
//  Created by Muhammed Okumuş on 16.05.2020.
//  Copyright 2020 Muhammed Okumuş. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>  
#include <unistd.h>     // getopt(), ftruncate(), access(), read(), fork(), close(), _exit()
#include <fcntl.h>      // open(), O_CREAT, O_RDWR, O_RDONLY
#include <pthread.h>    // all the pthread stuff
#include <semaphore.h>  // sem_init(), sem_wait(), sem_post(), sem_destroy()


#define STOCK 1024
#define CHEFS 6
#define errExit(msg) do{ perror(msg); print_usage(); exit(EXIT_FAILURE); } while(0)

struct info {
    int chef_no;
    char need1;
    char need2;
    int have1;
    int have2;
};

sem_t sem_stock_access;
sem_t sem_desserts;
int dessert_ready = 0;
int desserts_sold = 0;
int desserts_made = 0;
char stock[STOCK];
int wholesaler_done = 0;

// Function Prototypes
void print_usage(void);
char* get_name(char c);
void* chef(void* data);
void transfer(char c1, char c2);
int try_take(char c);



int main(int argc, char* argv[]){
    char filepath[255], buf[4];
    int option, fd;
    pthread_t thread_ids[CHEFS];
    struct info *chef_data[CHEFS];

    //Parse command line input ======================================================================
    while((option = getopt(argc, argv, "i:")) != -1){ //get option from the getopt() method
        switch(option){
            case 'i':
                snprintf(filepath,255,"%s",optarg);
                break;
            case ':':
                printf("Unknown option or needs value: %c\n", optopt);
                print_usage();
                errExit("Unknown option 1");
            case '?': //used for some unknown options
                printf("Unknown option or needs value: %c\n", optopt);
                print_usage();
                errExit("Unknown option 2");;
        }
    }

    for(; optind < argc; optind++) //when some extra arguments are passed
        fprintf(stderr,"Given extra arguments: %s\n", argv[optind]);

    if (access(filepath, F_OK | R_OK) == -1) errExit("Input file not accessable");
    printf("Wholesaler input path : %s\n", filepath);
    // ====================================================================== Parse command line input

    // Initilize thread data ==========================================
    if (sem_init(&sem_stock_access, 0, 1) == -1) errExit("sem_init @main - sem_stock_access");
    if (sem_init(&sem_desserts, 0, 0) == -1) errExit("sem_init @main - sem_desserts");
    // Initilize shared stock
    for(int i = 0; i < STOCK; i++){
        stock[i] = '-';
    }

    for(int i = 0; i < CHEFS; i++){
        chef_data[i] = malloc(sizeof(struct info));
        chef_data[i]->chef_no = i;
        chef_data[i]->have1 = 0;
        chef_data[i]->have2 = 0;
        switch(i){
            case 0:
                chef_data[i]->need1 = 'M';
                chef_data[i]->need2 = 'F';
                break;
            case 1:
                chef_data[i]->need1 = 'M';
                chef_data[i]->need2 = 'W';
                break;
            case 2:
                chef_data[i]->need1 = 'M';
                chef_data[i]->need2 = 'S';
                break;
            case 3:
                chef_data[i]->need1 = 'F';
                chef_data[i]->need2 = 'W';
                break;
            case 4:
                chef_data[i]->need1 = 'F';
                chef_data[i]->need2 = 'S';
                break;
            case 5:
                chef_data[i]->need1 = 'W';
                chef_data[i]->need2 = 'S';
                break;      
            default:
                errExit("Program supports upto 6 chefs maximum");   
        }
        //Create thread for the chef
        pthread_create(&thread_ids[i], NULL, chef, chef_data[i]); 
    }


    // ========================================= Initilize thread data 

    // Do wholesaler things ==========================================
    if( (fd = open(filepath, O_RDONLY)) == -1) errExit("open @main");
    while(read(fd, buf, 3) == 3){
        printf("the wholesaler delivers %s and %s\n", get_name(buf[0]), get_name(buf[1]));
        transfer(buf[0], buf[0]);

        if(dessert_ready > 0){
            printf("the wholesaler is waiting for the dessert\n");
            if (sem_wait(&sem_desserts) == -1) errExit("sem_wait @main - sem_desserts");
            dessert_ready--;
            desserts_sold++;
        }

        printf("the wholesaler has obtained the dessert and left to sell it\n");
        
    }
    while(dessert_ready > 0){
        printf("the wholesaler is waiting for the dessert\n");
        if (sem_wait(&sem_desserts) == -1) errExit("sem_wait @main - sem_desserts");
        dessert_ready--;
        desserts_sold++;
    }
    wholesaler_done++;
    printf("wholesaler done supplying - GOODBYE!!!\n");
    // ========================================== Do wholesaler things

    // Join threads and free resources =================================
    for(int i = 0; i < CHEFS; i++){
        pthread_join(thread_ids[i], NULL);
        free(chef_data[i]);
    }
    sem_destroy(&sem_stock_access);
    sem_destroy(&sem_desserts);
    //  ================================= Join threads and free resources

    printf("Desserts made: %d\n", desserts_made);
    printf("Desserts sold: %d\n", desserts_sold);
    printf("Stocks:\n%s\n",stock);


    exit(0);
}

void print_usage(void){
    printf("========================================\n");
    printf("Usage:\n"
           "./gullac  [-i file path]\n");
    printf("========================================\n");
}


/*
    @parameter c: M,F,W or S character, uppercase matters
    @return     : (m)ilk, (f)lour, etc. as char array
*/
char* get_name(char c){
    switch(c){
        case 'M': return "milk";
        case 'F': return "flour";
        case 'W': return "walnuts";
        case 'S': return "sugar";
        default : errExit("invalid ingredient");
    }
}

/*
    Do chef things, exit when wholesaler is done.
    @parameter data: struct info pointer as declerad above
    @return        : NULL
*/
void* chef(void* data){
    struct info *info = data;

    while(wholesaler_done == 0){

        if(info->have1 == 1 && info->have2 == 1){
            if (sem_post(&sem_desserts) == -1) errExit("sem_post @chef - sem_desserts");
            dessert_ready++;
            printf("chef%d is preparing the dessert\n",info->chef_no);
            
            info->have1 = 0;
            info->have2 = 0;
            desserts_made++;    
        }

        if (sem_wait(&sem_stock_access) == -1) errExit("sem_wait @chef - sem_stock_access");

        if(info->have1 == 0 && info->have2 == 0)
            printf("chef%d is waiting for %s and %s\n", info->chef_no, get_name(info->need1), get_name(info->need2)); 
        else if(info->have1 == 0)
            printf("chef%d is waiting for %s\n", info->chef_no, get_name(info->need1)); 
        else if(info->have2 == 0)
            printf("chef%d is waiting for %s\n", info->chef_no, get_name(info->need2)); 

        //printf("%s\n",stock);
        if(info->have1 == 0 && try_take(info->need1) == 0){
            info->have1 = 1;
            printf("chef%d has taken the %s\n", info->chef_no, get_name(info->need1)); 
        }
        else if(info->have2 == 0 && try_take(info->need2) == 0){
            info->have2 = 1;
            printf("chef%d has taken the %s\n", info->chef_no,get_name(info->need2)); 
        }


        if (sem_post(&sem_stock_access) == -1) errExit("sem_post @chef - sem_stock_access");
                
    }
    printf("Wholesaler said goodbye to chef%d - GOODBYE!!!\n",info->chef_no);
    pthread_exit(NULL);
}


/*
    Transfer 2 items to global stock[] array, overflow not checked
    Items are placed at first empty spots found
    @parameter c1: item 1, upper case ingredient
    @parameter c2: item 2, upper case ingredient
*/
void transfer(char c1, char c2){
    if (sem_wait(&sem_stock_access) == -1) errExit("sem_wait @transfer - sem_stock_access");

    int i = 0;
    for(i = 0; i < STOCK; i++){
        if(stock[i] == '-'){
            stock[i] = c1;
            break;
        }
    }
    for(int j = i+1; j < STOCK; j++){
        if(stock[j] == '-'){
            stock[j] = c2;
            break;
        }
    }
    if (sem_post(&sem_stock_access) == -1) errExit("sem_wait @transfer- sem_stock_access");
}

/*
    Try to take an item from the stock[] array
    @on_success : replace the item(char) taken from the array with '-'
                  and @return 0
    @on_failure : @return 1
*/
int try_take(char c){
    for(int i = 0; i < STOCK; i++){
        if(stock[i] == c){
            stock[i] = '-';
            return 0;
        }
    }
    return 1;
}