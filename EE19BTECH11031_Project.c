#include <stdio.h> 
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>

#define MAX_NODES 10000	//Total number of nodes in the graph
#define MAX_EDGES 3000	//An upper limit on the maximum number of edges a node can have

int t_max = 300;	//The number of days for which the simulation is carried on
int time_passed = 0;	//A variable to keep track of the number of days that have passed

int adj_matrix[MAX_NODES][MAX_NODES];	//The adjacency matrix

//Defining a node in the graph
typedef struct{
 char status;	//Stores the status of the node
 int pred_inf_time;	//Stores the time at which the node is presently predicted to get infected
 int rec_time;	//Stores the time at which the node is going to recover
}node;

//Defining an event to be put in the priority queue
typedef struct _event{ 
	int time; //Stores the time at which the event takes place
	char action;	//Stores the character which represents the action of the event
	int node_index;	//Stores the index of the node which is concerned with the event
	struct _event* next; //Pointer to the next event in the priority queue
}event; 

//Defining each member of the lists of susceptible, infected and recovered nodes
typedef struct _member{
	int node_index;	//Stores the index of the node present in that member of the list
	struct _member* next;	//Pointer to the next member in the list
	struct _member* prev;	//Pointer to the previous member in the list
	}member;

member* S = NULL;	//Head of the list of susceptible nodes
member* I = NULL;	//Head of the list of infected nodes
member* R = NULL;	//Head of the list of recovered nodes
int S_size = 0;	//Size of the list of susceptible nodes
int I_size = 0;	//Size of the list of infected nodes
int R_size = 0;	//Size of the list of recovered nodes

//Function which flips a coin which has 'prob' probability of showing heads and returns true if it results in a head and false otherwise
//The value of prob can have a non-zero value only upto the first two decimal places since we are multiplying by 100
bool coinToss(float prob){
	int temp = (rand()%100) + 1;
	
	if(temp<=(prob*100.0))
		return true;
	
	else
		return false;
	}

//Creates a new event that occurs at t, whose action is 'act' and concerns the node with index 'index' and returns a pointer to it
event* newEvent(int t, char act, int index) 
{ 
	event* temp = (event*)malloc(sizeof(event)); 
	temp->time = t; 
	temp->action = act;
	temp->node_index = index; 
	temp->next = NULL; 

	return temp; 
}

//Checks if the priority queue is empty and returns true in that case
bool isEmpty_queue(event** head) 
{ 
	if((*head) == NULL)
		return true;
	
	else
		return false;
}

//Removes the head of the priority queue
void pop(event** head) 
{ 
	event* temp = *head; 
	(*head) = (*head)->next; 
	free(temp);
	temp = NULL; 
} 

//Adds an event which occurs at t, whose action is 'act' and concerns the node with index 'index' at its appropriate place in the queue
//The priority queue is such that it has increasing order of times from head to tail
void push(event** head, int t, char act, int index)
{ 	event* temp = newEvent(t,act,index); 
	
	//If the queue is empty
	if(isEmpty_queue(head)){
		temp->next = NULL;
		(*head) = temp;
	}
	
	else{
			event* start = (*head); 
		
		//If the event has to become the new head
		if ((*head)->time > t) {  
			temp->next = *head; 
			(*head) = temp; 
		}
		
		else{ 
			//Searching for the right place to insert the new event
			while (start->next != NULL && 
				start->next->time <= t) { 
				start = start->next; 
			} 
 
		temp->next = start->next; 
		start->next = temp; 
		}
	}
}

//Creates a new member of a list which contains the node with index 'index'
member* newMember(int index){
	member* temp = (member*)malloc(sizeof(member));
	temp->node_index = index;
	
	return temp;
	}

//Checks if the list is empty and returns true in that case
bool isEmpty_list(member** head){
	if((*head) == NULL)
		return true;
	
	else
		return false;
	}

//Returns the pointer to the member of the list which contains the node with index 'index'
member* find(member** head, int index){
	member* temp = *head;
	while(temp!=NULL&&temp->node_index!=index){
		temp = temp->next;
		}
	
	return temp;
	}

//Deletes the member of the list which contains the node with index 'index'
void delete_member(member** head, int index, int* size){
	member* temp = find(head,index);
	
	//If the member to be deleted is the head of the list
	if(temp->prev==NULL){
		*head = (*head)->next;
		
		if(*head!=NULL)
			(*head)->prev = NULL;
		
		free(temp);
		temp = NULL;
		}
	
	else{
		temp->prev->next = temp->next;
		if(temp->next!=NULL){
			temp->next->prev = temp->prev;
			}
		free(temp);
		temp = NULL;
		}
	
	(*size)--;
	}

//Inserts a member into the list which contains the node with index 'index' at the end of the list
void insert_member(member** head, int index, int* size){
	//If the list is empty
	if(isEmpty_list(head)){
		*head = newMember(index);
		(*head)->next = NULL;
		(*head)->prev = NULL;
		}
	
	else{
		member* temp = *head;
		//Finding the last element of the list
		while(temp->next!=NULL){
			temp = temp->next;
			}
		
		temp->next = newMember(index);
		temp->next->prev = temp;
		temp->next->next = NULL;
		}
	
	(*size)++;
	}

//Determines whether the source transmits the virus to the target and if yes, adds that transmission event to the priority queue
void find_trans(event** Q, node* list, float trans_prob, int source, int target){
	if(list[target].status=='S'){
		int inf_time = 0;
		//Tossing a coin to determine whether the transmission takes place or not
		while(!coinToss(trans_prob)){
			inf_time++;
			}
		inf_time++;
		inf_time = time_passed + inf_time;
		
		//Checking whether the transmission event is plausible or not and if yes, adding it to the priority queue
		if(inf_time<list[source].rec_time&&inf_time<list[target].pred_inf_time&&inf_time<t_max){
			push(Q,inf_time,'T',target);
			list[target].pred_inf_time = inf_time;
			}
		}
	}

//Function called in case of a transmission event
//list is the array of nodes, matrix is the adjacency matrix and 'index' is the index of the concerned node
void process_trans_SIR(event** Q, node* list, int matrix[MAX_NODES][MAX_NODES], int index, float trans_prob, float rec_prob){
	//Editing the lists of S and I nodes
	delete_member(&S,index,&S_size);
	insert_member(&I,index,&I_size);
	list[index].status = 'I';	//Changing the concerned node's status
	
	//Determining the recovery time of the node by tossing a toin
	int r_time = 0;
	while(!coinToss(trans_prob)){
		r_time++;
		}
	r_time++;
	list[index].rec_time = time_passed + r_time;
	
	//Determining if the recovery event is plausible
	if(list[index].rec_time<t_max){
		push(Q,list[index].rec_time,'R',index);
		}
	
	//Determing is there are any transmission events from the concerned node to its neighbours in the future
	for(int i = 0; i<MAX_NODES; i++){
		if(matrix[index][i]==1){
			find_trans(Q,list,trans_prob,index,i);
			}
		}
	
	}

//Function called in case of a recovery event
//list is the array of nodes and 'index' is the index of the concerned node
void process_rec_SIR(node* list, int index){
	//Editing the lists of I and R nodes
	delete_member(&I,index,&I_size);
	insert_member(&R,index,&R_size);
	list[index].status = 'R';	//Changing the concerned node's status
	}
 
int main(){
	FILE *fp;
	fp = fopen("data.dat","w");
	
	float trans_prob = 0.5;	//Probability of a transmission event
	float rec_prob = 0.2;	//Probability of a recovery event
	int i,j,linkedNode;
    int numberOfNodes = MAX_NODES;	//Total number of nodes in the graph
    
    printf("Total number of nodes : %d\n", numberOfNodes);
	
    srand (time(NULL));
    int maxNumberOfEdges = (rand() % MAX_EDGES) + 1;	//Maximum number of edges a node can have
    
    printf("Maximum number of edges each node can have : %d\n", maxNumberOfEdges);
    
    node* node_list = (node*)malloc(sizeof(node)*numberOfNodes);	//Creating the array of nodes
    
    //Assigning initial values to all the nodes in the array
    for(i = 0; i<numberOfNodes; i++){
		node_list[i].status = 'S';
		node_list[i].pred_inf_time = INFINITY;
		node_list[i].rec_time = INFINITY;
		}
	
    //Assigning initial value as 0 to all elements in the adjacency matrix
    for(i = 0; i<numberOfNodes; i++){
		for(j = 0; j<numberOfNodes; j++){
			adj_matrix[i][j] = 0;
			}
		}
	
	int nodeCounter = 0;
	int edgeCounter = 0;
	
	//Creating edges between the nodes randomly
	for(nodeCounter = 0; nodeCounter<numberOfNodes; nodeCounter++){
		edgeCounter = 0;
		
		//Checking if there are already any edges to the concerned node
		for(i = 0; i<numberOfNodes; i++){
			if(adj_matrix[nodeCounter][i]==1)
				edgeCounter++;
			}
		
		while(edgeCounter < maxNumberOfEdges){
			if(rand()%2==1){
				linkedNode = rand()%numberOfNodes;	//Randomly selecting the node it is to be linked to
				
				//Since the graph is undirected, creating two links between the two nodes
				adj_matrix[nodeCounter][linkedNode] = 1;
				adj_matrix[linkedNode][nodeCounter] = 1;
				}
			edgeCounter++;
			}
		}
	
	//Since a node cannot be linked to itself, setting the diagonal elements to 0
	for(i = 0; i<numberOfNodes; i++){
		adj_matrix[i][i] = 0;
		}
	
	//Printing the links thus obtained
	/*
	for(i = 0; i<numberOfNodes; i++){
		printf("%d : ", i);
		for(j = 0; j<numberOfNodes; j++){
			if(adj_matrix[i][j]==1)
				printf("%d ", j);
			}
		printf("\n\n");
		}*/
	
	event* queue = NULL;	//Declaring the head of the priority queue
	
	//Adding all the nodes to the S list initially
	for(i = 0; i<numberOfNodes; i++){
		insert_member(&S,i,&S_size);
		}
	
	//Randomly generating the number of initially infected nodes
	int initial_infected_size = (rand()%10) + 1;
	printf("Number of initially infected nodes : %d\n\n", initial_infected_size);
	int initial_infected[initial_infected_size];
	
	//randomly selecting the initially infected nodes
	for(i = 0; i<initial_infected_size; i++){
		initial_infected[i] = rand()%numberOfNodes;
		}
	
	//Adding the transmission events of the initially infected nodes to the priority queue
	for(i = 0; i<initial_infected_size; i++){
		push(&queue,0,'T',initial_infected[i]);
		node_list[initial_infected[i]].pred_inf_time = 0;
		}
	
	//Executing the events of the priority queue until it is empty
	while(queue!=NULL){
		
		time_passed = queue->time;
		
		//Transmission event
		if(queue->action=='T'){
			if(node_list[queue->node_index].status=='S')
				process_trans_SIR(&queue,node_list,adj_matrix,queue->node_index,trans_prob,rec_prob);
			}
		
		//Recovery event
		else
			process_rec_SIR(node_list,queue->node_index);
		
		//Printing the sizes of the S,I,R lists each day to the terminal and to the dat file
		if(queue->next!=NULL){
			if(queue->time<queue->next->time){
				while(time_passed<queue->next->time){
					printf("day %d : S = %d, I = %d, R = %d\n", time_passed, S_size, I_size, R_size);
					fprintf(fp,"%d %d %d %d\n", time_passed, S_size, I_size, R_size);
					time_passed++;
					}
				}
			}
		
		else{
			while(time_passed<=t_max){
				printf("day %d : S = %d, I = %d, R = %d\n", time_passed, S_size, I_size, R_size);
				fprintf(fp,"%d %d %d %d\n", time_passed, S_size, I_size, R_size);
				time_passed++;
				}
			}
		
		pop(&queue);	//Popping the event from the priority queue after its execution
		}
	
	fclose(fp);
	return 0;
	} 
