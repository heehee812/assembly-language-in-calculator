#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define MAX_LENGTH 100

// Token / AST kinds
enum {
	// Non-operators
	LPar, RPar, // parentheses
	Value, Variable,
	// Operators
	/// Precedence 1
	PostInc, PostDec,
	/// Precedence 2
	PreInc, PreDec, Plus, Minus,
	/// Precedence 3
	Mul, Div, Rem,
	/// Precedence 4
	Add, Sub,
	/// Precedence 14
	Assign
};
const char TYPE[20][20] = {
	"LPar", "RPar",
	"Value", "Variable",
	"PostInc", "PostDec",
	"PreInc", "PreDec", "Plus", "Minus",
	"Mul", "Div", "Rem",
	"Add", "Sub",
	"Assign"
};
typedef struct _TOKEN {
	int kind;
	int param; // Value, Variable, or Parentheses label
	struct _TOKEN *prev, *next;
} Token;
typedef struct _AST {
	int type;
	int val; // Value or Variable
	struct _AST *lhs, *rhs, *mid;
} AST;
// Utility Interface

// Function called when an unexpected expression occurs.
void err();
// Used to create a new Token.
Token *new_token(int kind, int param);
// Used to create a new AST node.
AST *new_AST(Token *mid);
// Convert Token linked list into array form.
int list_to_arr(Token **head);
// Use to check if the kind can be determined as a value section.
int isBinaryOperator(int kind);
// Pass "kind" as parameter. Return true if it is an operator kind.
int isOp(int x);
// Pass "kind" as parameter. Return true if it is an unary kind.
// unary contains increment, decrement, plus, and minus.
int isUnary(int x);
// Pass "kind" as parameter. Return true if it is a parentheses kind.
int isPar(int x);
// Pass "kind" as parameter. Return true if it is a plus or minus.
int isPlusMinus(int x);
// Pass "kind" as parameter. Return true if it is an operand(value or variable).
int isOperand(int x);
// Return the precedence of a kind. If doesn't have precedence, return -1.
int getOpLevel(int kind);
// Given the position of left parthesis, find the right part in range [tar,r]. If not found, return -1.
int findParPair(Token *arr, int tar, int r);
// Parse the expression the the next section. A section means a Value, Variable, or a parenthesis pair.
int nextSection(Token *arr, int l, int r);
// Used to find a appropriate operator that split the expression half.
int find_Tmid(Token *arr, int l, int r);
// Determine the memory location of variable
int var_memory(AST *ast);


// Debug Interface

// Print the AST. You may set the indent as 0.
void AST_print(AST *head, int indent);

// Main Function

char input[MAX_LENGTH];

// Convert the inputted string into multiple tokens.
Token *lexer(char *in);
// Use tokens to build the binary expression tree.
AST *parser(Token *arr, int l, int r);
// Checkif the expression(AST) is legal or not.
void semantic_check(AST *now);
// Generate the ASM.
void codegen(AST *ast);
void turn_to_reg(AST **ast);

int reg=0;
AST store[]={{0, -1}, {0, -1}, {0, -1}};
AST *first;

int main() {
	
	while(fgets(input, MAX_LENGTH, stdin) != NULL) {
		for(int i=0; i<3; i++)
		{
			store[i].type=0;
		}
		// build token list by lexer
		Token *content = lexer(input);
		// convert token list into array
		int length = list_to_arr(&content);
		// build abstract syntax tree by parser
		AST *ast_root = parser(content, 0, length-1);
        //AST_print(ast_root, 0);
		// check if the syntax is correct
		semantic_check(ast_root);
		turn_to_reg(&ast_root);
		// generate the assembly
		first=ast_root;
		codegen(ast_root);
		int val=-1;
		for(int i=0; i<3; i++)
		{
			if(store[i].val!=-1)
			{
				if(store[i].val>val)
					val=store[i].val;
			}
		}
		//printf("val=%d\n", val);
		reg=val+1;
	}
	return 0;
}

Token *lexer(char *in) {
	Token *head = NULL, *tmp = NULL;
	Token **now = &head, *prev = NULL;
	int par_cnt = 0;
	for(int i = 0; in[i]; i++) {
		if(in[i] == ' ' || in[i] == '\n')
			continue;

		else if('x' <= in[i] && in[i] <= 'z')
			(*now) = new_token(Variable, in[i]);

		else if(isdigit(in[i])) {
			int val = 0, oi = i;
			for(; isdigit(in[i]); i++)
				val = val * 10 + (in[i] - '0');
			i--;
			// Detect illegal number inputs such as "01"
			if(oi != i && in[oi] == '0')
				err();
			(*now) = new_token(Value, val);
		}

		else {
			switch(in[i]) {
				case '+':
					if(in[i+1] == '+') { // '++'
						tmp = prev;
						while(tmp != NULL && tmp->kind == RPar) tmp = tmp->prev;
						if(tmp != NULL && tmp->kind == Variable)
							(*now) = new_token(PostInc, -1);
						else (*now) = new_token(PreInc, -1);
						i++;
					}
					else { // '+'
						if(prev == NULL || isOp(prev->kind) || prev->kind == LPar || isPlusMinus(prev->kind))
							(*now) = new_token(Plus, -1);
						else (*now) = new_token(Add, -1);
					}
					break;
				case '-':
					if(in[i+1] == '-') { // '--'
						tmp = prev;
						while(tmp != NULL && tmp->kind == RPar) tmp = tmp->prev;
						if(tmp != NULL && tmp->kind == Variable)
							(*now) = new_token(PostDec, -1);
						else (*now) = new_token(PreDec, -1);
						i++;
					}
					else { // '-'
						if(prev == NULL || isOp(prev->kind) || prev->kind == LPar || isPlusMinus(prev->kind))
							(*now) = new_token(Minus, -1);
						else (*now) = new_token(Sub, -1);
					}
					break;
				case '*':
					(*now) = new_token(Mul, -1);
					break;
				case '/':
					(*now) = new_token(Div, -1);
					break;
				case '%':
					(*now) = new_token(Rem, -1);
					break;
				case '(':
					(*now) = new_token(LPar, par_cnt++);
					break;
				case ')':
					(*now) = new_token(RPar, --par_cnt);
					break;
				case '=':
					(*now) = new_token(Assign, -1);
					break;
				default:
					err();
			}
		}
		(*now)->prev = prev;
		if(prev != NULL) prev->next = (*now);
		prev = (*now);
		now = &((*now)->next);
	}
	return head;
}

AST *parser(Token *arr, int l, int r) {
	if(l > r) return NULL;
	// covered in parentheses
	if(r == findParPair(arr, l, r)) {
		AST *res = new_AST(arr+l);
		res->mid = parser(arr, l+1, r-1);
		return res;
	}

	int mid = find_Tmid(arr, l, r);
	AST *newN = new_AST(arr + mid);
	
	if(l == r) {
		if(newN->type <= Variable)
        {
            return newN;
        }
		else err();
	}

	if(getOpLevel(arr[mid].kind) == 1) // a++, a--
		newN->mid = parser(arr, l, mid-1);
	// TODO: Implement the remaining parser part.
	// hint: else if(other op type?) then do something ..etc
    else if(getOpLevel(arr[mid].kind)==2)
    {
        newN->mid=parser(arr, mid+1, r);
    }
    else if(getOpLevel(arr[mid].kind)==3)
    {
        newN->lhs=parser(arr, l, mid-1);
        newN->rhs=parser(arr, mid+1, r);
    }
    else if(getOpLevel(arr[mid].kind)==4)
    {
        newN->lhs=parser(arr, l, mid-1);
        newN->rhs=parser(arr, mid+1, r);
    }
    else if(getOpLevel(arr[mid].kind)==14)
    {
        newN->lhs=parser(arr, l, mid-1);
        newN->rhs=parser(arr, mid+1, r);
    }

	return newN;
}

void semantic_check(AST *now) {
	if(isUnary(now->type) || isPar(now->type)) {
		if(now->lhs != NULL || now->rhs != NULL)
			err();
		if(now->mid == NULL)
			err();
		if(isUnary(now->type)) {
			AST *tmp = now->mid;
			if(isPar(tmp->type)) {
				while(isPar(tmp->type))
					tmp = tmp->mid;
			}
			if(isPlusMinus(now->type)) {
				if(isUnary(tmp->type));
				else if(isOperand(tmp->type));
				else err();
			}
			else if(tmp->type != Variable)
				err();
		}

		semantic_check(now->mid);
	}
	// TODO: Implement the remaining semantic check part.
	// hint: else if(other op type?) then do something ...etc

    else if(isOp(now->type))
    {
		if(now->lhs == NULL || now->rhs == NULL)
			err();
        if(now->mid!=NULL)
            err();
		if(now->type==Assign)
		{
			if((now->lhs)->type!=LPar&&(now->lhs)->type!=Variable)
				err();
			while((now->lhs)->type==LPar)
			{
				AST *del=now->lhs;
				now->lhs=del->mid;
				free(del);
			}
			if((now->lhs)->type!=Variable)
			{
				err();
			}
			semantic_check(now->rhs);
		}
		else
		{
			semantic_check(now->lhs);
        	semantic_check(now->rhs);
		}
    }
    else if(isOperand(now->type))
    {
        return ;
    }
    else if(now==NULL)
        return ;
}

void turn_to_reg(AST** ast)
{
	if((*ast)->type==Assign)
		turn_to_reg(&((*ast)->rhs));
	else{
		if((*ast)->lhs!=NULL)
		{
			//printf("in lhs\n");
			turn_to_reg(&((*ast)->lhs));
		}
		if((*ast)->mid!=NULL)
		{
			//printf("in mid\n");
			if(getOpLevel((*ast)->type)==1)
			{
				if(((*ast)->mid)->val=='x')
					if(store[0].val!=-1)
					{
						if(store[1].val==store[0].val||store[2].val==store[0].val)
							store[0].val=reg++;
					}
				else if(((*ast)->mid)->val=='y')
					if(store[1].val!=-1)
					{
						if(store[0].val==store[1].val||store[2].val==store[1].val)
							store[1].val=reg++;
					}
				else
					if(store[2].val!=-1)
					{
						if(store[0].val==store[2].val||store[1].val==store[2].val)
							store[2].val=reg++;
					}
			}
			turn_to_reg(&((*ast)->mid));
		}
		if((*ast)->rhs!=NULL)
		{
			turn_to_reg(&((*ast)->rhs));
		}
		if((*ast)->type==Variable)
		{
			if((*ast)->val=='x')
			{
				if(store[0].val==-1)
					store[0].val=reg++;
				if(store[0].type==0)
				{
					printf("load r%d [0]\n", store[0].val);
					++store[0].type;
				}
				(*ast)->val=store[0].val;
			}
			else if((*ast)->val=='y')
			{
				if(store[1].val==-1)
					store[1].val=reg++;
				if(store[1].type==0)
				{
					printf("load r%d [4]\n", store[1].val);
					++store[1].type;
				}
				(*ast)->val=store[1].val;
			}
			else
			{
				if(store[2].val==-1)
					store[2].val=reg++;
				if(store[2].type==0)
				{
					printf("load r%d [8]\n", store[2].val);
					++store[2].type;
				}
				(*ast)->val=store[2].val;
			}
		}
	}
	return ;
}

void codegen(AST *ast)
{

	if (ast->type==PreInc||ast->type==PreDec)
	{
		if(ast->type==PreInc)
			printf("add r%d r%d 1\n", (ast->mid)->val, (ast->mid)->val);
		else if(ast->type==PreDec)
			printf("sub r%d r%d 1\n", (ast->mid)->val, (ast->mid)->val);
		else;
		if(ast==first)
		{
			printf("store ");
			if((ast->mid)->val==store[0].val)
				printf("[0] ");
			else if((ast->mid)->val==store[1].val)
				printf("[1] ");
			else
				printf("[2] ");
			printf("r%d\n", (ast->mid)->val);
		}
		ast->type=(ast->mid)->type;
		ast->val=(ast->mid)->val;
		free(ast->mid);
		ast->mid=NULL;
		return ;
	}
	else if(ast->type==LPar)
	{
		if(isUnary((ast->mid)->type))
		{
			AST *tmp=ast->mid;
			ast->type=(ast->mid)->type;
			ast->val=(ast->mid)->val;
			ast->mid=tmp->mid;
			free(tmp);
		}
		if(isBinaryOperator((ast->mid)->type))
		{
			AST *tmp=ast->mid;
			ast->type=(ast->mid)->type;
			ast->val=(ast->mid)->val;
			ast->lhs=tmp->lhs;
			ast->rhs=tmp->rhs;
			free(tmp);
			ast->mid=NULL;
		}
		codegen(ast);
		return ;
	}
	else if(isPlusMinus(ast->type))
	{
		int i=0;
		if(ast->type==Minus)
			++i;
		while((ast->mid)->type!=Variable&&(ast->mid)->type!=Value)
		{
			if((ast->mid)->type==LPar)
			{
				AST *ignore=ast->mid;
				ast->mid=ignore->mid;
				free(ignore);
			}
			if((ast->mid)->type!=Variable&&(ast->mid)->type!=Value)
			{
				AST *del=ast->mid;
				if(del->type==Minus)
					++i;
				ast->mid=del->mid;
				free(del);
			}
		}
		
		if(i%2==0)
		{
			ast->type=(ast->mid)->type;
			ast->val=(ast->mid)->val;
		}
		else
		{
			ast->type=Minus;
			if((ast->mid)->type==Value)
			{
				ast->type=Value;
				ast->val=-(ast->mid)->val;
			}
			else
			{
				if(ast->val==-1)
					ast->val=reg++;
				printf("sub r%d 0 r%d\n", ast->val, (ast->mid)->val);
				ast->type=Variable;
			}	
		}
		free(ast->mid);
		ast->mid=NULL;
		return ;	
	}
	else if(isBinaryOperator(ast->type)&&ast->type!=Assign)
	{
		if((ast->lhs)->type==Value&&(ast->rhs)->type==Value)
		{
			switch(ast->type)
			{
				case(Add):
					ast->val=(ast->lhs)->val+(ast->rhs)->val;
					break;
				case(Sub):
					ast->val=(ast->lhs)->val-(ast->rhs)->val;
					break;
				case(Mul):
					ast->val=(ast->lhs)->val*(ast->rhs)->val;
					break;
				case(Div):
					ast->val=(ast->lhs)->val/(ast->rhs)->val;
					break;
				case(Rem):
					ast->val=(ast->lhs)->val%(ast->rhs)->val;
					break;
			}
			ast->type=Value;
		}
		else
		{
			codegen(ast->lhs);
			codegen(ast->rhs);
			if((ast->lhs)->type==Value&&(ast->rhs)->type==Value)
			{
				switch(ast->type)
				{
					case(Add):
						ast->val=(ast->lhs)->val+(ast->rhs)->val;
						break;
					case(Sub):
						ast->val=(ast->lhs)->val-(ast->rhs)->val;
						break;
					case(Mul):
						ast->val=(ast->lhs)->val*(ast->rhs)->val;
						break;
					case(Div):
						ast->val=(ast->lhs)->val/(ast->rhs)->val;
						break;
					case(Rem):
						ast->val=(ast->lhs)->val%(ast->rhs)->val;
						break;
				}
				ast->type=Value;
				free(ast->lhs);
				free(ast->rhs);
				ast->lhs=NULL;
				ast->rhs=NULL;
				return ;
			}
			else if(ast->type==Value)
				return ;
			else if(ast->type==Add)
				printf("add ");
			else if(ast->type==Sub)
				printf("sub ");
			else if(ast->type==Mul)
				printf("mul ");
			else if(ast->type==Div)
				printf("div ");
			else if(ast->type==Rem)
				printf("rem ");
			else;

			if((ast->lhs)->type!=Variable&&(ast->lhs)->type!=Value)
			{
				if(ast->val==-1)
					ast->val=(ast->lhs)->val;	
			}
			else if((ast->rhs)->type!=Variable&&(ast->rhs)->type!=Value)
			{
				if(ast->val==-1)
					ast->val=(ast->rhs)->val;
			}
			else
			{
				if(ast->val==-1)
					ast->val=reg++;
			}
			printf("r%d ", ast->val);
			
			if((ast->lhs)->type==Variable)
				printf("r%d ", (ast->lhs)->val);
			else if((ast->lhs)->type==Value)
				printf("%d ", (ast->lhs)->val);
			else if((ast->lhs)->type==PostInc||(ast->lhs)->type==PostDec)
				printf("r%d ", ((ast->lhs)->mid)->val);
			else
				printf("r%d ", (ast->lhs)->val);
			if((ast->rhs)->type==Variable)
				printf("r%d \n",(ast->rhs)->val);
			else if((ast->rhs)->type==Value)
				printf("%d \n", (ast->rhs)->val);
			else if((ast->rhs)->type==PostInc||(ast->rhs)->type==PostDec)
				printf("r%d \n", ((ast->rhs)->mid)->val);
			else
				printf("r%d \n", (ast->rhs)->val);
			if((ast->lhs)->type==PostInc||(ast->lhs)->type==PostDec)
			{
				if((ast->lhs)->type==PostInc)
				{
					printf("add r%d r%d 1\n", ((ast->lhs)->mid)->val, ((ast->lhs)->mid)->val);
					(ast->lhs)->type=((ast->lhs)->mid)->type;
					(ast->lhs)->val=((ast->lhs)->mid)->val;
					free((ast->lhs)->mid);
					(ast->lhs)->mid=NULL;
				}
				else if((ast->lhs)->type==PostDec)
				{
					printf("sub r%d r%d 1", ((ast->lhs)->mid)->val, ((ast->lhs)->mid)->val);
					(ast->lhs)->type=((ast->lhs)->mid)->type;
					(ast->lhs)->val=((ast->lhs)->mid)->val;
					free((ast->lhs)->mid);
					(ast->lhs)->mid=NULL;
				}
				if((ast->lhs)->val==store[0].val)
					printf("store [0] r%d\n", (ast->lhs)->val);
				else if((ast->lhs)->val==store[1].val)
					printf("store [4] r%d\n", (ast->lhs)->val);
				else
					printf("store [8] r%d\n", (ast->lhs)->val);	
			}
			else;

			if((ast->rhs)->type==PostInc||(ast->rhs)->type==PostDec)
			{
				if((ast->rhs)->type==PostInc)
				{
					printf("add r%d r%d 1", ((ast->rhs)->mid)->val, ((ast->rhs)->mid)->val);
					(ast->rhs)->type=((ast->rhs)->mid)->type;
					(ast->rhs)->val=((ast->rhs)->mid)->val;
					free((ast->rhs)->mid);
					(ast->rhs)->mid=NULL;
				}
				else if((ast->rhs)->type==PostDec)
				{
					printf("sub r%d r%d 1", ((ast->rhs)->mid)->val, ((ast->rhs)->mid)->val);
					(ast->rhs)->type=((ast->rhs)->mid)->type;
					(ast->rhs)->val=((ast->rhs)->mid)->val;
					free((ast->rhs)->mid);
					(ast->rhs)->mid=NULL;
				}
				if((ast->rhs)->val==store[0].val)
					printf("store [0] r%d\n", (ast->rhs)->val);
				else if((ast->rhs)->val==store[1].val)
					printf("store [4] r%d\n", (ast->rhs)->val);
				else
					printf("store [8] r%d\n", (ast->rhs)->val);	
			}
			else;
		}	
		free(ast->lhs);
		free(ast->rhs);
		ast->lhs=NULL;
		ast->rhs=NULL;
		return ;
	}
	else if(ast->type==Assign)
	{
		if((ast->lhs)->type==Variable)
		{	
			if((ast->lhs)->val=='x')
			{
				if(store[0].val!=-1)
					if((ast->rhs)->type!=Value&&(ast->rhs)->type!=Variable)
					{
						ast->val=store[0].val;
						(ast->rhs)->val=store[0].val;
					}
			}
			else if((ast->lhs)->val=='y')
			{
				if(store[1].val!=-1)
					if((ast->rhs)->type!=Value&&(ast->rhs)->type!=Variable)
					{
						ast->val=store[1].val;
						(ast->rhs)->val=store[1].val;
					}
			}
			else
			{
				if(store[2].val!=-1)
					if((ast->rhs)->type!=Value&&(ast->rhs)->type!=Variable)
					{
						ast->val=store[2].val;
						(ast->rhs)->val=store[2].val;
					}
			}
			if((ast->rhs)->type==LPar)
				((ast->rhs)->mid)->val=(ast->rhs)->val;

		}
		codegen(ast->rhs);
		if((ast->lhs)->type==Variable)
		{
			if((ast->rhs)->type==Value)
			{
				int val=reg++;
				printf("mul r%d %d 1\n", val, (ast->rhs)->val);
				(ast->rhs)->val=val;
				(ast->rhs)->type=Variable;
			}
			printf("store ");
			if((ast->lhs)->val=='x')
				printf("[0] ");
			else if((ast->lhs)->val=='y')
				printf("[4] ");
			else
				printf("[8] ");
			if(getOpLevel((ast->rhs)->type)==1)
			{
				printf("r%d\n", ((ast->rhs)->mid)->val);
			}
			else if(getOpLevel((ast->rhs)->type)==14)
			{
				if((ast->rhs)->rhs!=NULL)
				{
					while(getOpLevel((ast->rhs)->type)==14)
					{
							AST *del=ast->rhs;
							ast->rhs=del->rhs;
							free(del);
					}
					if(getOpLevel((ast->rhs)->type==1))
					{
						printf("r%d\n", ((ast->rhs)->mid)->val);
						if((ast->rhs)->type==PostInc)
							printf("add r%d r%d 1\n", ((ast->rhs)->mid)->val, ((ast->rhs)->mid)->val);
						else
							printf("sub r%d r%d 1\n", ((ast->rhs)->mid)->val, ((ast->rhs)->mid)->val);
						if(((ast->rhs)->mid)->val==store[0].val)
							printf("store [0] r%d\n", ((ast->rhs)->mid)->val);
						else if(((ast->rhs)->mid)->val==store[1].val)
							printf("store [4] r%d\n", ((ast->rhs)->mid)->val);
						else
							printf("store [8] r%d\n", ((ast->rhs)->mid));
						(ast->rhs)->type=((ast->rhs)->mid)->type;
						(ast->rhs)->val=((ast->rhs)->mid)->val;
						free((ast->rhs)->mid);
						(ast->rhs)->mid=NULL;
					}
				}
				else
				{
					printf("r%d\n", (ast->rhs)->val);
					if((ast->lhs)->val=='x')
						store[0].val=(ast->rhs)->val;
					else if((ast->lhs)->val=='y')
						store[1].val=(ast->rhs)->val;
					else
						store[2].val=(ast->rhs)->val;
				}
			}
			else
			{
				printf("r%d\n", (ast->rhs)->val);	
				if((ast->lhs)->val=='x')
					store[0].val=(ast->rhs)->val;
				else if((ast->lhs)->val=='y')
					store[1].val=(ast->rhs)->val;
				else
					store[2].val=(ast->rhs)->val;
			}
			if(ast->val==-1)
			{
				if(getOpLevel((ast->rhs)->type)==1)
					(ast->rhs)->val=((ast->rhs)->mid)->val;
				ast->val=(ast->rhs)->val;
			}
			if(getOpLevel((ast->rhs)->type)==1)
			{
				free(ast->lhs);
				ast->lhs=NULL;
			}
			else
			{
				free(ast->lhs);
				free(ast->rhs);
				ast->lhs=NULL;
				ast->rhs=NULL;
			}
		}
		return ;
	}
	else//value, variable, postinc, postdec
	{
		if(ast=first)
		{
			if(getOpLevel(ast->type)==1)
			{
				if(ast->type==PostInc)
					printf("add r%d r%d 1\n", (ast->mid)->val, (ast->mid)->val);
				else if(ast->type==PostDec)
					printf("sub r%d r%d 1\n", (ast->mid)->val, (ast->mid)->val);
				printf("store ");
				if((ast->mid)->val==store[0].val)
					printf("[0] ");
				else if((ast->mid)->val==store[1].val)
					printf("[4] ");
				else if((ast->mid)->val==store[2].val)
					printf("[8] ");
				else;
				printf("r%d\n", (ast->mid)->val);
			}
		}
		else
			return ;
	}
}
	// TODO: Implement your own codegen.
	// You may modify the pass parameter(s) or the return type as you wish.

void err() {
	puts("Compile Error!");
	exit(0);
}

Token *new_token(int kind, int param) {
	Token *res = (Token*)malloc(sizeof(Token));
	res->kind = kind;
	res->param = param;
	res->prev = res->next = NULL;
	return res;
}

AST* new_AST(Token *mid) {
	AST *newN = (AST*)malloc(sizeof(AST));
	newN->lhs = newN->mid = newN->rhs = NULL;
	newN->type = mid->kind;
	newN->val = mid->param;
	return newN;
}
int list_to_arr(Token **head) {
	int res = 0;
	Token *now = (*head), *t_head = NULL, *del;
	while(now!=NULL) {
		res++;
		now = now->next;
	}
	now = (*head);
	t_head = (Token*)malloc(sizeof(Token)*res);
	for(int i = 0; i < res; i++) {
		t_head[i] = (*now);
		del = now;
		now = now->next;
		free(del);
	}
	(*head) = t_head;
	return res;
}
int isBinaryOperator(int kind) {
	int res = getOpLevel(kind);
	if(res >= 3) return 1;
	return 0;
}

int isOp(int x) {
	return Mul <= x && x <= Assign;
}

int isUnary(int x) {
	return PostInc <= x && x <= Minus;
}

int isPar(int x) {
	return LPar <= x && x<= RPar;
}

int isPlusMinus(int x) {
	if(x == Plus) return 1;
	if(x == Minus) return 1;
	return 0;
}

int isOperand(int x) {
	if(x == Value) return 1;
	if(x == Variable) return 1;
	return 0;
}

int getOpLevel(int kind) {
	int res;
	if(kind <= Variable) res = -1;
	else if(kind <= PostDec) res = 1;
	else if(kind <= Minus) res = 2;
	else if(kind <= Rem) res = 3;
	else if(kind <= Sub) res = 4;
	else if(kind <= Assign) res = 14;
	else res = -1;
	return res;
}

int findParPair(Token *arr, int tar, int r) {
	if(arr[tar].kind != LPar) return -1;
	for(int i = tar + 1; i <= r; i++)
		if(arr[i].kind == RPar)
			if(arr[i].param == arr[tar].param)
				return i;
	return -1;
}

int nextSection(Token *arr, int l, int r) {
	int res = l;
	if(arr[l].kind == LPar) {
		res = findParPair(arr, l, r);
		if(res == -1) err();
	}
	return res + 1;
}

int find_Tmid(Token *arr, int l, int r) {
	int big = l;
	for(int i = l; i <= r;) {
		if(getOpLevel(arr[big].kind) <= getOpLevel(arr[i].kind)) {
			if(isPlusMinus(arr[big].kind) && isPlusMinus(arr[i].kind));
			else if(getOpLevel(arr[big].kind) != 14)
            {
				big = i;
            }
		}
		i = nextSection(arr, i, r);
	}
	return big;
}

int var_memory(AST *ast) {
	while(ast->type != Variable)
		ast = ast->mid;
	
	switch(ast->val) {
		case 'x': return 0;
		case 'y': return 4;
		case 'z': return 8;
		default: 
			err();
			return -1;
	}
}

void AST_print(AST *head, int indent) {
	if(head == NULL) return;
	const char kind_only[] = "<%s>\n";
	const char kind_para[] = "<%s>, <%s = %d>\n";
	for(int i=0;i<indent;i++) printf("  ");
	switch(head->type) {
		case LPar:
		case RPar:
		case PostInc:
		case PostDec:
		case PreInc:
		case PreDec:
		case Plus:
		case Minus:
		case Mul:
		case Div:
		case Rem:
		case Add:
		case Sub:
		case Assign:
			printf(kind_only, TYPE[head->type]);
			break;
		case Value:
			printf(kind_para, TYPE[head->type], "value", head->val);
			break;
		case Variable:
			printf(kind_para, TYPE[head->type], "name", head->val);
			break;
		default:
			puts("Undefined AST Type!");
	}
	AST_print(head->lhs, indent+1);
	AST_print(head->mid, indent+1);
	AST_print(head->rhs, indent+1);
}