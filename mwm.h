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
/*void make_list(client *head, int n, int *a);*/
void push_back(client *head, Window win, Window frame);
void push_front(client **head, Window win, Window frame);
void push(client *head, int index, Window win, Window frame);
void pop_back(client *head);
void pop_front(client **head);
void pop(client *head, int index);
void change_back(client *head, Window win, Window frame);
void change_front(client *head, Window win, Window frame);
void change(client *head, int index, Window win, Window frame);
void bubblesort(client *head);

/* function implementation */
void print_list(client *head) {
	client *current = head;
	while(current != NULL) {
		printf("Window: %ld Frame: %ld", current->win, current->frame);
		current = current->next;
		if(current == NULL) {
			printf("\n");
		}
	}
}

/*void make_list(client *head, int n, int *a) {
	client *current = head;
	for(int i = 0; i < n; i++) {
		current->win = a[i];
		if(i != n-1) {
			current->next = malloc(sizeof(client));
		}
		else {
			current->next = NULL;
		}
		current = current->next;
	}
}*/

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


/*void bubblesort(node *head) {
	for(node *curi = head; curi->next != NULL; curi = curi->next) {
		for(node *curj = curi->next; curj != NULL; curj = curj->next) {
			if(curi->win > curj->win) {
				int tmp = curi->win;
				curi->win = curj->win;
				curj->win = tmp;
			}
		}
	}
}*/

/*int main() {
	node *head = NULL;
	head = malloc(sizeof(node));
	if(head == NULL){
		return 1;
	}
	int n;
	printf("Number of elements: ");
	scanf("%d", &n);
	printf("Elements: ");
	int a[n];
	for(int i = 0; i < n; i++) {
		scanf("%d", &a[i]);
	}
	make_list(head, n, a);
	print_list(head);
	pop_front(&head);
	print_list(head);
	pop(head, 2);
	print_list(head);
	push_front(&head, 100);
	print_list(head);
	push_front(&head, 200);
	print_list(head);
	push_back(head, 300);
	print_list(head);
	push_back(head, 400);
	print_list(head);
	push_front(&head, 500);
	print_list(head);
	bubblesort(head);
	print_list(head);
	return 0;
}*/
