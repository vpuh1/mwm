#include <stdio.h>
#include <stdlib.h>

/* function declaration */
void print_list(Client *head);
void push_back(Client *head, Window win, Window frame, int ws_num);
void push_front(Client **head, Window win, Window frame, int ws_num);
void push(Client *head, int index, Window win, Window frame, int ws_num);
void pop_back(Client *head);
void pop_front(Client **head);
void pop(Client *head, int index);

/* function implementation */
void print_list(Client *head) {
	Client *current = head;
	while(Current != NULL) {
		fprintf(stderr, "Window: %ld Frame: %ld Workspace: %d", current->win, current->frame, current->ws_num);
		current = current->next;
		if(current == NULL) {
			fprintf(stderr, "\n");
		}
	}
}

void push_back(Client *head, Window win, Window frame, int ws_num) {
	Client *current = head;
	while(current->next != NULL) {
		current = current->next;
	}
	current->next = malloc(sizeof(Client));
	current->next->win = win;
	current->next->frame = frame;
	current->next->ws_num = ws_num;
	current->next->next = NULL;
}

void push_front(Client **head, Window win, Window frame, int ws_num) {
	Client *new_node;
	new_node = malloc(sizeof(Client));
	new_node->next = *head;
	new_node->win = win;
	new_node->frame = frame;
	new_node->ws_num = ws_num;
	*head = new_node;
}

void push(Client *head, int index, Window win, Window frame, int ws_num) {
	Client *current = head;
	for(int i = 0; i < index-1; i++) {
		if(current->next != NULL) {
			current = current->next;
		}
	}
	if(current->next == NULL || index == 0) {
		return;
	}
	Client *tmp = current->next;
	current->next = malloc(sizeof(Client));
	current->next->win = win;
	current->next->frame = frame;
	current->next->ws_num = ws_num;
	current->next->next = tmp;
}

void pop_back(Client *head) {
	if(head->next == NULL) {
		free(head);
		return;
	}
	Client *current = head;
	while(current->next->next != NULL) {
		current = current->next;
	}
	free(current->next);
	current->next = NULL;
}

void pop_front(Client **head) {
	Client *next_node = NULL;
	if(head == NULL){
		return;
	}
	next_node = (*head)->next;
	free(*head);
	*head = next_node;
}

void pop(Client *head, int index) {
	Client *current = head;
	for(int i = 0; i < index-1; i++){
		if(current->next != NULL){
			current = current->next;
		}
	}
	Client *tmp = current->next->next;
	free(current->next);
	current->next = tmp;
}
