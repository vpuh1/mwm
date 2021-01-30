#include <stdio.h>
#include <stdlib.h>

/* linked list */
typedef struct client {
	Window win;	
	Window frame;	
	struct client *next;
} client;

/* function declaration */
void print_list(client *head);
void push_back(client *head, Window win, Window frame);
void push_front(client **head, Window win, Window frame);
void push(client *head, int index, Window win, Window frame);
void pop_back(client *head);
void pop_front(client **head);
void pop(client *head, int index);
void change_back(client *head, Window win, Window frame);
void change_front(client *head, Window win, Window frame);
void change(client *head, int index, Window win, Window frame);

/* function implementation */
void print_list(client *head) {
	client *current = head;
	while(current != NULL) {
		fprintf(stderr, "Window: %ld Frame: %ld", current->win, current->frame);
		current = current->next;
		if(current == NULL) {
			fprintf(stderr, "\n");
		}
	}
}

void push_back(client *head, Window win, Window frame) {
	client *current = head;
	while(current->next != NULL) {
		current = current->next;
	}
	current->next = malloc(sizeof(client));
	current->next->win = win;
	current->next->frame = frame;
	current->next->next = NULL;
}

void push_front(client **head, Window win, Window frame) {
	client *new_node;
	new_node = malloc(sizeof(client));
	new_node->next = *head;
	new_node->win = win;
	new_node->frame = frame;
	*head = new_node;
}

void push(client *head, int index, Window win, Window frame) {
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

void change_front(client *head, Window win, Window frame) {
	head->win = win;
	head->frame = frame;
}

void change_back(client *head, Window win, Window frame) {
	client *current = head;
	while(current->next != NULL){
		current = current->next;
	}
	current->win = win;
	current->frame = frame;
}

void change(client *head, int index, Window win, Window frame) {
	client *current = head;
	for(int i = 0; i < index-1; i++) {
		if(current->next != NULL) {
			current = current->next;  
		}
	}
	current->next->win = win;
	current->next->frame = frame;
}
