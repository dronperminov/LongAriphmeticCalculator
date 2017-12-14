#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "colors.h"

const char* text_color = GREEN;
const char* error_color = BOLDRED;

// структура для хранения лексем - чисел, знаков операций и скобок
typedef struct lexeme_t {
	char *value; // значение лексемы
	int length; // и длина строки (чтобы постоянно не вычислять strlen)
	int sign; // знак (только для чисел)
} lexeme_t;

// структура для хранения дерева выражения
typedef struct tree_t {
	lexeme_t lexeme; // собственно сама лексема

	struct tree_t *left; // указатель на левое поддерево
	struct tree_t *right; // указатель на правое поддерево
} tree_t;

// проверка, что символ является арифметической операцией
int is_ariphmetic(char c) {
	return c == '+' || c == '-' || c == '*' || c == '/' || c == '^';
}

// проверка, что символ является скобкой
int is_bracket(char c) {
	return c == '(' || c == ')';
}

// проверка, что символ является числом
int is_digit(char c) {
	return c >= '0' && c <= '9';
}

// определение приоритета операции (сложение и вычитание менее приорететны, чем умножение и деление)
int priority(lexeme_t c) {
	switch (c.value[0]) {
	case '+':
	case '-':
		return 0;

	case '*':
	case '/':
		return 1;

	case '^':
		return 2;

	default:
		return 3;
	}
}

// получение массива лексем с получение количества считанных лексем для дальнейшей обработки выражения
lexeme_t* parse(int *size, FILE *f) {
	*size = 0; // изначально считано ноль лексем
	int capacity = 1; // максимально доступное число лексем для массива
	lexeme_t *lexemes = (lexeme_t *)malloc(sizeof(lexeme_t)); // выделим память под одну лексему

	char c = fgetc(f); // считываем первый символ

	// читаем до конца строки (признак окончания выражения)
	while (c != '\n' && c != EOF) {
		// если символ - арифметическая операция или скобка, то длина строки равна 1, а строка соответствует считанному символу операции или скобки
		if (is_ariphmetic(c) || is_bracket(c)) {
			lexemes[*size].length = 1;
			lexemes[*size].sign = 1;

			if (c == '+')
				lexemes[*size].value = "+";
			else if (c == '-')
				lexemes[*size].value = "-";
			else if (c == '*')
				lexemes[*size].value = "*";
			else if (c == '/')
				lexemes[*size].value = "/";
			else if (c == '^')
				lexemes[*size].value = "^";
			else if (c == '(')
				lexemes[*size].value = "(";
			else
				lexemes[*size].value = ")";

			(*size)++;

			if (*size >= capacity) {
				capacity *= 2;
				lexemes = (lexeme_t *)realloc(lexemes, capacity * sizeof(lexeme_t)); // если число лексем стало больше, чем может вместить массив, то увеличиваем размер в два раза
			}

			c = fgetc(f);
		}
		else if (is_digit(c)) { // иначе, если считали цифру, то
			int s_size = 0; // размер строки ноль
			int s_capacity = 1; // вместимость строки сначала равна 1
			char *s = (char *)malloc(s_capacity * sizeof(char)); // заводим строку для числа

			do {
				s[s_size++] = c; // сохраняем символ цифры

				if (s_size >= s_capacity) {
					s_capacity *= 2;
					s = (char *)realloc(s, s_capacity * sizeof(char)); // если некуда сохранять символы, то расширяем аналогично массиву
				}

				c = fgetc(f);
			} while (is_digit(c)); // считываем число до тех пор, пока идут цифры

			s[s_size] = '\0';

			lexemes[*size].sign = 1;

			// если встретили структур вида [не число] - [число] или выражение началось с минус числа
			if ((*size > 2 && lexemes[*size - 1].value[0] == '-' && is_ariphmetic(lexemes[*size - 2].value[0])) || (*size == 1 && lexemes[0].value[0] == '-')) {
				(*size)--; // то переходим к предыдущей лексеме

				lexemes[*size].sign = -1; // и делаем её отрицательной
			}

			lexemes[*size].value = s; // сохраняем строку в массиве
			lexemes[*size].length = s_size; // и длину строки

			(*size)++; // увеличиваем счётчик количества лексем

			if (*size >= capacity) {
				capacity *= 2;
				lexemes = (lexeme_t *)realloc(lexemes, capacity * sizeof(lexeme_t)); // если число лексем стало больше, чем может вместить массив, то увеличиваем размер в два раза
			}
		}
		else if (c == ' ') {
			c = fgetc(f);
		}
		else { // это что-то странное, и нужно заканчивать работу
			printf("%sError! Unknown symbol %s'%c'%s in expression.%s\n", error_color, RESET, c, error_color, RESET);

			// очищаем выделенную динамическую память
			for (int i = 0; i < *size; i++)
				if (is_digit(lexemes[i].value[0]))
					free(lexemes[i].value);

			free(lexemes);
			*size = 0; // обнуляем число считанных лексем

			while (c != '\n' && c != EOF)
				c = fgetc(f); // считываем строку до конца

			return NULL; // возвращем нулевой указатель
		}
	}

	return lexemes; // дошли до конца строки, возвращаем указатель на массив лексем
}

int check(lexeme_t *lexemes, int size) {
	int brackets = 0;

	for (int i = 0; i < size; i++) {
		lexeme_t lex = lexemes[i];

		if (lexemes[i].value[0] == '(')
			brackets++;
		else if (lexemes[i].value[0] == ')')
			brackets--;

		if (brackets < 0) {
			printf("%sBrackets are disbalanced%s\n", error_color, RESET);
			return 0;
		}

		if (i == 0) {
			if (!is_digit(lex.value[0]) && lex.value[0] != '(') {
				printf("%sExpression must begin with a number or '('%s\n", error_color, RESET);
				return 0;
			}
		}
		else {
			if (i == size - 1) {
				if (!is_digit(lex.value[0]) && lex.value[0] != ')') {
					printf("%sExpression must end with a number or ')'%s\n", error_color, RESET);
					return 0;
				}
			}

			if (is_ariphmetic(lexemes[i - 1].value[0]) && !is_digit(lex.value[0]) && lex.value[0] != '(') {
				printf("%sAfter operation can be only number or '('%s\n", error_color, RESET);
				return 0;
			}

			if (lexemes[i - 1].value[0] == '(' && !is_digit(lexemes[i].value[0]) && lexemes[i].value[0] != '(') {
				printf("%sAfter '(' can be only '(' or number%s\n", error_color, RESET);
				return 0;
			}

			if (is_digit(lexemes[i - 1].value[0]) && !is_ariphmetic(lex.value[0]) && lex.value[0] != ')') {
				printf("%sAfter number can be only operation or ')'%s\n", error_color, RESET);
				return 0;
			}
		}
	}

	if (brackets != 0) {
		printf("%sBrackets are disbalanced%s\n", error_color, RESET);
		return 0;
	}

	return 1;
}

// формирование дерева выражения (на листах операнды, а в корнях знаки операций)
tree_t* make_tree(lexeme_t *lexemes, int first, int last) {
	tree_t* tree = (tree_t *)malloc(sizeof(tree_t)); // выделяем память под элемент дерева

	int k = 0, minPriority = 3;
	int brackets = 0;

	for (int i = first; i <= last; i++) {
		if (lexemes[i].value[0] == '(')
			brackets++;
		else if (lexemes[i].value[0] == ')')
			brackets--;
		else if (brackets <= 0) {
			int curPriority = priority(lexemes[i]);

			if (curPriority <= minPriority) {
				minPriority = curPriority;
				k = i;
			}
		}
	}

	if (minPriority == 3 && lexemes[first].value[0] == '(' && lexemes[last].value[0] == ')') {
		free(tree); // если встретили выражение в скобках, то удаляем текущий лист и формируем дерево от концов скобок (с тем, что внутри скобок)

		return make_tree(lexemes, first + 1, last - 1); // возвращаем дерево выражения, находящегося внутри скобок
	}
	else if (minPriority == 3) {
		tree->lexeme = lexemes[first];
		tree->left = NULL;
		tree->right = NULL;

		return tree;
	}

	tree->lexeme = lexemes[k]; // запоминаем лексему
	tree->left = make_tree(lexemes, first, k - 1); // формируем дерево из левой части
	tree->right = make_tree(lexemes, k + 1, last); // формируем дерево из правой части

	return tree;
}

// проверка, что число a меньше числа b
int less(lexeme_t a, lexeme_t b) {
	if (a.sign != b.sign)
		return a.sign == -1;

	// если длины не равны
	if (a.length != b.length)
		return (a.length < b.length) ^ (a.sign == -1); // меньше число с меньшей длинной

	int i = 0;

	// ищем разряд, в котором значения отличаются
	while (i < a.length && a.value[i] == b.value[i])
		i++;

	// если разряд найден, то меньше число с меньшей цифрой
	return (i < a.length) && ((a.value[i] < b.value[i]) ^ (a.sign == -1));
}

lexeme_t add(lexeme_t a, lexeme_t b);
lexeme_t sub(lexeme_t a, lexeme_t b);
lexeme_t mult(lexeme_t a, lexeme_t b);
lexeme_t divide(lexeme_t a, lexeme_t b);
lexeme_t power(lexeme_t a, lexeme_t b);

// операция суммирования чисел a и b
lexeme_t add(lexeme_t a, lexeme_t b) {
	if (a.sign != b.sign) { // если знаки разные, то это вычитание
		if (a.sign == 1) {
			b.sign = 1;
			return sub(a, b);
		}
		else {
			a.sign = 1;
			return sub(b, a);
		}
	}

	lexeme_t result;

	result.length = 1 + (a.length > b.length ? a.length : b.length);  // длина суммы равна максимуму из двух длин + 1 из-за возможного переноса разряда
	result.value = (char *)malloc((result.length + 1) * sizeof(char)); // строковый массив для выполнения операции сложения
	result.sign = a.sign;

	result.value[result.length - 1] = result.value[result.length] = '\0';

	for (int i = 0; i < result.length - 1; i++) {
		int j = result.length - 1 - i;
		int digit_a = ((i < a.length) ? (a.value[a.length - 1 - i] - '0') : 0);
		int digit_b = ((i < b.length) ? (b.value[b.length - 1 - i] - '0') : 0);

		result.value[j] += digit_a + digit_b; // выполняем сложение разрядов
		result.value[j - 1] = result.value[j] / 10; // выполняем перенос в следущий разряд, если он был
		result.value[j] %= 10; // оставляем только единицы от возможного переноса
		result.value[j] += '0'; // превращаем цифру в символ цифры
	}

	result.value[0] += '0';

	int index = 0; // определяем, с какого момента начинается ненулевые символы (если там вообще не ноль)
	while (result.value[index] == '0' && result.length - index > 1)
		index++;

	for (int i = 0; i < result.length - index; i++)
		result.value[i] = result.value[index + i]; // сдвигаим символы в начало строки

	result.length -= index; // уменьшаем длину лексемы
	result.value[result.length] = '\0';
	result.value = (char *)realloc(result.value, (result.length + 1) * sizeof(char)); // перераспределяем память
	return result;
}

// операция вычитания из числа a числа b
lexeme_t sub(lexeme_t a, lexeme_t b) {
	if (b.sign == 1 && a.sign == -1) {
		a.sign = -1;
		b.sign = -1;

		return add(a, b);
	}

	if (b.sign == -1) {
		b.sign = 1;
		return add(a, b);
	}

	lexeme_t result;

	result.length = a.length > b.length ? a.length : b.length; // длина результата не превысит максимума длин чисел
	result.sign = less(a, b) ? -1 : 1;

	int isNegRes = result.sign != 1;

	// массивы аргументов
	int *digits_a = (int *)malloc(result.length * sizeof(int));
	int *digits_b = (int *) malloc(result.length * sizeof(int));

	digits_a[0] = digits_b[0] = 0; // обнуляем нулевые элементы массивов

	result.value = (char *)malloc((result.length + 1) * sizeof(char)); // строковый массив для результата
	result.value[result.length - 1] = result.value[result.length] = '\0'; // устанавливаем символ окончания строки

	int sign = (2 * isNegRes - 1); // получаем числовое значение знака результата

	for (int i = 0; i < result.length - 1; i++) {
		digits_a[i] += (i < a.length) ? (a.value[a.length - 1 - i] - '0') : 0; // формируем разряды
		digits_b[i] += (i < b.length) ? (b.value[b.length - 1 - i] - '0') : 0; // из строк аргументов

		digits_b[i + 1] = -isNegRes; // в зависимости от знака занимаем или не занимаем
		digits_a[i + 1] = isNegRes - 1; // 10 у следующего разряда

		result.value[result.length - 1 - i] += 10 + sign * (digits_b[i] - digits_a[i]);
		result.value[result.length - 1 - i - 1] = result.value[result.length - 1 - i] / 10;
		result.value[result.length - 1 - i] = result.value[result.length - 1 - i] % 10 + '0';
	}

	// выполняем операцию с последним разрядом
	digits_a[result.length - 1] += (result.length - 1 < a.length) * (a.value[0] - '0');
	digits_b[result.length - 1] += (result.length - 1 < b.length) * (b.value[0] - '0');

	// записываем в строку последний разряд
	result.value[0] += sign * (digits_b[result.length - 1] - digits_a[result.length - 1]) + '0';

	// освобождаем динамическую память, выделенную под массив цифр
	free(digits_a);
	free(digits_b);

	int index = 0; // определяем, с какого момента начинается ненулевые символы (если там вообще не ноль)
	while (result.value[index] == '0' && result.length - index > 1)
		index++;

	for (int i = 0; i < result.length - index; i++)
		result.value[i] = result.value[index + i]; // сдвигаим символы в начало строки

	result.length -= index; // уменьшаем длину лексемы
	result.value[result.length] = '\0';
	result.value = (char *)realloc(result.value, (result.length + 1) * sizeof(char)); // перераспределяем память
	return result;
}

// операция умножения двух чисел
lexeme_t mult(lexeme_t a, lexeme_t b) {
	lexeme_t result;

	result.length = a.length + b.length + 1; // резульат влезет в сумму длин + 1 из-за возможного переноса
	result.sign = a.sign * b.sign;

	// массивы аргументов
	int *digits_a = (int *)malloc(result.length * sizeof(int));
	int *digits_b = (int *)malloc(result.length * sizeof(int));

	result.value = (char *) malloc((result.length + 1) * sizeof(char)); // строковый массив для результата
	result.value[result.length] = '\0'; // устанавливаем символ окончания строки

	// заполняем массивы инверсной записью чисел (с ведущими нулями)
	for (int i = 0; i < result.length; i++) {
		digits_a[i] = (i < a.length) ? (a.value[a.length - 1 - i] - '0') : 0;
		digits_b[i] = (i < b.length) ? (b.value[b.length - 1 - i] - '0') : 0;
		result.value[i] = 0;
	}

	// выполняем умножение "в столбик""
	for (int i = 0; i < a.length; i++) {
		for (int j = 0; j < b.length; j++) {
			result.value[result.length - 1 - (i + j)] += digits_a[i] * digits_b[j];
			result.value[result.length - 1 - (i + j + 1)] += result.value[result.length - 1 - (i + j)] / 10;
			result.value[result.length - 1 - (i + j)] %= 10;
		}
	}

	free(digits_a);
	free(digits_b);

	// переписываем результат в строку
	for (int i = 0; i < result.length; i++)
		result.value[result.length - 1 - i] += '0';

	int index = 0; // определяем, с какого момента начинается ненулевые символы (если там вообще не ноль)
	while (result.value[index] == '0' && result.length - index > 1)
		index++;

	for (int i = 0; i < result.length - index; i++)
		result.value[i] = result.value[index + i]; // сдвигаим символы в начало строки

	result.length -= index; // уменьшаем длину лексемы
	result.value[result.length] = '\0';
	result.value = (char *)realloc(result.value, (result.length + 1) * sizeof(char)); // перераспределяем память
	return result;
}

// операция деления числа a на b в столбик
lexeme_t divide(lexeme_t a, lexeme_t b) {
	lexeme_t result;

	if (a.value[0] == '0' || less(a, b)) {
		result.length = 1;
		result.sign = 1;

		result.value = (char *)malloc(2 * sizeof(char));
		result.value[0] = '0';
		result.value[1] = '\0';

		return result; // ноль делить можно на всё, но смысл?
	}

	if (b.value[0] == '1' && b.length == 1) {
		result.length = a.length;
		result.sign = a.sign * b.sign;

		result.value = (char *)calloc((a.length + 1), sizeof(char));

		for (int i = 0; i < result.length; i++)
			result.value[i] = a.value[i];

		return result; // делить на 1 можно, но смысл?
	}

	lexeme_t tmp;
	tmp.length = b.length;
	tmp.sign = 1;
	tmp.value = (char *)calloc((tmp.length + 1), sizeof(char));
	for (int i = 0; i < tmp.length; i++)
		tmp.value[i] = b.value[i];

	int length = a.length; // получаем длину делимого
	int index = 0; // стартуем с нулевого индекса

	lexeme_t v; // строка подчисла (которое делится на делитель в столбик)
	v.length = 0;
	v.sign = 1;
	v.value = (char *)calloc(a.length + 1, sizeof(char));

	// формируем начальное число для деления
	while (less(v, tmp) && index < length)
		v.value[v.length++] = a.value[index++];

	lexeme_t div; // строка результата деления
	div.length = 0;
	div.value = (char *)calloc(a.length + 1, sizeof(char));

	do {
		int count = 0; // результат деления подчисла на делитель

		// если можем разделить, то делим
		if (!less(v, tmp)) {
			lexeme_t mod;
			mod.length = v.length;
			mod.sign = v.sign;
			mod.value = (char*)calloc(mod.length + 1, sizeof(char));
			for (int i = 0; i < mod.length; i++)
				mod.value[i] = v.value[i];

			while (!less(mod, tmp)) {
				lexeme_t mod2 = sub(mod, tmp);
				free(mod.value);
				mod = mod2;

				count++;
			}

			for (int i = 0; i < mod.length; i++)
				v.value[i] = mod.value[i];

			v.length = mod.length;
			free(mod.value);
		}

		div.value[div.length++] = count + '0'; // если не делили, то добавили ноль к результату, иначе добавили результат дедения

		if (index <= length) {
			if (v.value[0] == '0')
				v.length = 0;

			v.value[v.length++] = a.value[index++]; // формируем новое значение для подчисла
		}
	} while (index <= length);

	free(v.value);
	free(tmp.value);

	div.sign = a.sign * b.sign;

	return div;
}

// быстрое возведение числа a в степень b
lexeme_t power(lexeme_t a, lexeme_t b) {
	if ((b.length == 1 && b.value[0] == '0') || b.sign == -1) {
		lexeme_t result;
		result.length = 1;
		result.sign = 1;
		result.value = (char *)calloc(2, sizeof(char));
		result.value[0] = '1';

		return result;
	}

	if ((b.value[b.length - 1] - '0') % 2) {
		lexeme_t one = {"1", 1, 1};

		lexeme_t minus = sub(b, one);
		lexeme_t p1 = power(a, minus);
		lexeme_t p = mult(a, p1);

		free(p1.value);
		free(minus.value);

		return p;
	}

	lexeme_t two = {"2", 1, 1};
	lexeme_t div = divide(b, two);
	lexeme_t p = power(a, div);

	lexeme_t pp = mult(p, p);

	free(div.value);
	free(p.value);

	return pp;
}

// вычисление выражения с помощью дерева
lexeme_t calculate(tree_t *tree) {
	if (!tree->left) { // если дошли до листа
		lexeme_t lexeme = tree->lexeme; // запоминаем его значение
		free(tree); // освобождаем память из-под дерева

		return lexeme; // возвращаем лексему
	}

	lexeme_t left = calculate(tree->left); // получаем результат левой части
	lexeme_t right = calculate(tree->right); // получаем результат правой части
	lexeme_t result;

	switch (tree->lexeme.value[0]) { // определяем, что нужно сделать по знаку операции, находящемуся в текущей вершине дерева
	case '+':
		result = add(left, right); // если плюс, то складываем
		break;

	case '-':
		result = sub(left, right); // если минус, то вычитаем
		break;

	case '*':
		result = mult(left, right); // если умножение, то умножаем
		break;

	case '/':
		result = divide(left, right); // если деление, то делим
		break;

	case '^':
		result = power(left, right); // если возведение в степень, то возводим в степень
		break;
	}

	free(left.value);
	free(right.value);
	free(tree); // освобождаем память из-под дерева

	return result; // вовзращаем результат подсчитанного выражения
}

void fromFile() {
	char buf[256];
	char c;

	do {
		printf("%sEnter path to file: %s", text_color, RESET);
		scanf("%s%c", buf, &c); // спрашиваем и считываем путь к файлу

		FILE *f = fopen(buf, "r"); // открываем файл на чтение
		int exit = 0;

		while (!f && !exit) { // если не открылся, повторяем ввод или предлагаем выйти из программы
			printf("%sError during opening file %s'%s'%s. Is file here?%s\n", error_color, RESET, buf, error_color, RESET);
			printf("%sEnter path or type 'exit': %s", text_color, RESET);
			scanf("%s%c", buf, &c);

			exit = !strcmp(buf, "exit");

			if (!exit)
				f = fopen(buf, "r"); // если не выходим, то открываем новый файл
		}

		if (!exit) {
			while (!feof(f)) {
				int size;
				lexeme_t *lexemes = parse(&size, f); // получаем массив лексем

				if (size) { // если в выражении что-то есть
					for (int i = 0; i < size; i++)
						printf("%s%s", lexemes[i].sign == -1 ? "-" : "", lexemes[i].value);

					if (check(lexemes, size)) { // проверяем выражение на корректность
						tree_t *tree = make_tree(lexemes, 0, size - 1); // формируем дерево выражения
						lexeme_t result = calculate(tree); // вычисляем выражение, если получилось

						printf(" = %s%s\n", result.sign == -1 ? "-" : "", result.value);
						free(result.value);
					}
				}
				else {
					// очищаем выделенную динамическую память
					for (int i = 0; i < size; i++)
						if (is_digit(lexemes[i].value[0]))
							free(lexemes[i].value);
				}

				free(lexemes);
			}

			printf("%sType act ([repeat] / exit): %s", text_color, RESET);
			fgets(buf, sizeof(buf), stdin);
			printf("\n");
		}
	} while ((!strcmp(buf, "repeat\n") || !strcmp(buf, "\n")));
}

void fromConsole() {
	char buf[256];

	do {
		printf("%sEnter expression: %s", text_color, RESET); // если в консольном режиме, то просим ввести выражение

		int size;
		lexeme_t *lexemes = parse(&size, stdin); // получаем массив лексем из стандартного файла ввода

		if (size && lexemes) { // если в выражении что-то есть
			if (check(lexemes, size)) { // проверяем выражение на корректность
				for (int i = 0; i < size; i++)
					printf("%s%s ", lexemes[i].sign == -1 ? "-" : "", lexemes[i].value);

				tree_t *tree = make_tree(lexemes, 0, size - 1); // формируем дерево выражения
				lexeme_t result = calculate(tree); // вычисляем выражение, если получилось

				printf(" = %s%s\n", result.sign == -1 ? "-" : "", result.value);
				free(result.value);
			}
			else {
				// очищаем выделенную динамическую память
				for (int i = 0; i < size; i++)
					if (is_digit(lexemes[i].value[0]))
						free(lexemes[i].value);
			}

			free(lexemes);
		} else if (lexemes) {
			printf("%sempty expression!%s\n", error_color, RESET);
			free(lexemes);
		}

		printf("%sType act ([repeat] / exit): %s", text_color, RESET);
		fgets(buf, sizeof(buf), stdin);
		printf("\n");
	} while (!strcmp(buf, "repeat\n") || !strcmp(buf, "\n"));
}

int main() {
	char buf[256];

	printf("Select mode for program ([console] / file / exit): ");
	fgets(buf, sizeof(buf), stdin);

	if (!strcmp(buf, "exit\n"))
		return 0;

	while (1) {
		if (!strcmp(buf, "file\n")) {// если работаем через файл
			fromFile();
			break;
		}
		else if (!strcmp(buf, "console\n") || !strcmp(buf, "\n")) {
			fromConsole();
			break;
		}
		else if (!strcmp(buf, "exit\n")) {
			break;
		}
		else {
			printf("Uncknown mode. Type again ([console] / file / exit): ");
			fgets(buf, sizeof(buf), stdin);
		}
	}

	return 0;
}
