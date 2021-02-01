#include <stdio.h>
#include <stdlib.h>

/* linked list */
typedef struct client {
	Window win;	
	Window frame;	
	struct client *next;
	int ws_num;
	int w, h;
} client;

/* function declaration */
void print_list(client *head);
void push_back(client *head, Window win, Window frame, int ws_num);
void push_front(client **head, Window win, Window frame, int ws_num);
void push(client *head, int index, Window win, Window frame, int ws_num);
void pop_back(client *head);
void pop_front(client **head);
void pop(client *head, int index);

/* function implementation */
void print_list(client *head) {
	client *current = head;
	while(current != NULL) {
		fprintf(stderr, "Window: %ld Frame: %ld Workspace: %d", current->win, current->frame, current->ws_num);
		current = current->next;
		if(current == NULL) {
			fprintf(stderr, "\n");
		}
	}
}

void push_back(client *head, Window win, Window frame, int ws_num) {
	client *current = head;
	while(current->next != NULL) {
		current = current->next;
	}
	current->next = malloc(sizeof(client));
	current->next->win = win;
	current->next->frame = frame;
	current->next->ws_num = ws_num;
	current->next->next = NULL;
}

void push_front(client **head, Window win, Window frame, int ws_num) {
	client *new_node;
	new_node = malloc(sizeof(client));
	new_node->next = *head;
	new_node->win = win;
	new_node->frame = frame;
	new_node->ws_num = ws_num;
	*head = new_node;
}

void push(client *head, int index, Window win, Window frame, int ws_num) {
	client *current = head;
	for(int i = 0; i < index-1; i++) {
		if(current->next != NULL) {
			current = current->next;
		}
	}
	if(current->next == NULL || index == 0) {
		return;
	}
	client *tmp = current->next;
	current->next = malloc(sizeof(client));
	current->next->win = win;
	current->next->frame = frame;
	current->next->ws_num = ws_num;
	current->next->next = tmp;
}

void pop_back(client *head) {
	if(head->next == NULL) {
		free(head);
		return;
	}
	client *current = head;
	while(current->next->next != NULL) {
		current = current->next;
	}
	free(current->next);
	current->next = NULL;
}

void pop_front(client **head) {
	client *next_node = NULL;
	if(head == NULL){
		return;
	}
	next_node = (*head)->next;
	free(*head);
	*head = next_node;
}

void pop(client *head, int index) {
	client *current = head;
	for(int i = 0; i < index-1; i++){
		if(current->next != NULL){
			current = current->next;
		}
	}
	client *tmp = current->next->next;
	free(current->next);
	current->next = tmp;
}
