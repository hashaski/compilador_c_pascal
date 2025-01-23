/*
Projeto de Compiladores - Análise Semântica e Geração de Código

gcc -g -Og -Wall  .\compilador.c -o .\compilador

André Matteucci - 10403403 
Marcos Carvalho - 10401844
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Definição dos átomos léxicos
typedef enum {
    ERRO,
    IDENTIFICADOR,
    NUMERO,
    PROGRAM,
    BEGIN,
    END,
    IF,
    ELIF,
    FOR,
    READ,
    WRITE,
    SET,
    TO,
    OF,
    INTEGER,
    BOOLEAN,
    AND,
    OR,
    NOT,
    TRUE,
    FALSE,
    ABRE_PAR,
    FECHA_PAR,
    PONTO_VIRGULA,
    VIRGULA,
    PONTO,
    DOIS_PONTOS,
    MENOR,
    MENOR_IGUAL,
    IGUAL,
    DIFERENTE,
    MAIOR,
    MAIOR_IGUAL,
    MAIS,
    MENOS,
    MULT,
    DIVISAO,
    COMENTARIO,
    EOS
} TAtomo;

// Estrutura de informações sobre um átomo
typedef struct {
    TAtomo atomo;
    int linha;
    int atributo_numero;
    char atributo_ID[16];
} TInfoAtomo;

// Estrutura de informações sobre um símbolo
typedef struct {
    char identificador[16];
    int endereco;
} Simbolo;

// Constantes
#define MAX_SIMBOLOS 100
#define MAX_ROTULOS 100

//Variáveis globais
char *buffer;
int contaLinha = 1;
int memoria_extra = 0;
TInfoAtomo token;
FILE *arquivo;

// Mensagens de erro
const char *msgAtomo[] = {
    "ERRO", "IDENTIFICADOR", "NUMERO", "PROGRAM", "BEGIN", "END", "IF", "ELIF",
    "FOR", "READ", "WRITE", "SET", "TO", "OF", "INTEGER", "BOOLEAN", "AND",
    "OR", "NOT", "TRUE", "FALSE", "ABRE_PAR", "FECHA_PAR", "PONTO_VIRGULA",
    "VIRGULA", "PONTO", "DOIS_PONTOS", "MENOR", "MENOR_IGUAL", "IGUAL",
    "DIFERENTE", "MAIOR", "MAIOR_IGUAL", "MAIS", "MENOS", "MULT", "DIVISAO",
    "COMENTARIO", "EOS"
};

// Tabela de símbolos
Simbolo tabela_simbolos[MAX_SIMBOLOS];
int conta_simbolos = 0;

// Contador de rótulos
int contador_rotulos = 1;

// Declaração das funções
TInfoAtomo obter_atomo();
void analisador_sintatico();
void erro_sintatico(const char *esperado);
void erro_semantico(const char *mensagem);
void consome(TAtomo esperado);
void programa();


TInfoAtomo reconhece_identificador_ou_palavra_reservada();
TInfoAtomo reconhece_numero();
void ignora_espacos_e_comentarios();

void bloco();
void declaracao_de_variaveis();
void tipo();
void lista_variavel();
void comando_composto();
void comando();
void comando_atribuicao();
void comando_condicional();
void comando_repeticao();
void comando_entrada();
void comando_saida();
void expressao();
void expressao_simples();
void expressao_relacional();
void termo();
void fator();

int busca_tabela_simbolos(char *identificador);
void insere_tabela_simbolos(char *identificador);
int proximo_rotulo();

// Função principal
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <arquivo_fonte>\n", argv[0]);
        return 1;
    }

    arquivo = fopen(argv[1], "r");
    if (!arquivo) {
        printf("Erro ao abrir o arquivo %s\n", argv[1]);
        return 1;
    }

    // Carrega o conteúdo do arquivo no buffer
    fseek(arquivo, 0, SEEK_END);
    long tamanho = ftell(arquivo);
    fseek(arquivo, 0, SEEK_SET);
    buffer = (char *)malloc(tamanho + 1);
    fread(buffer, 1, tamanho, arquivo);
    buffer[tamanho] = '\0';
    fclose(arquivo);

    token = obter_atomo();
    printf("\tINPP\n");
    analisador_sintatico();
    printf("\tPARA\n");

    // Impressão da tabela de símbolos
    printf("\nTabela de simbolos:\n");
    for (int i = 0; i < conta_simbolos; i++) {
        printf("%-10s | Endereco: %d\n", tabela_simbolos[i].identificador, tabela_simbolos[i].endereco);
    }

    free(buffer);
    return 0;
}

// Ignora espaços em branco e comentários no código fonte
void ignora_espacos_e_comentarios() {
    while (*buffer == ' ' || *buffer == '\t' || *buffer == '\r' || *buffer == '\n' || *buffer == '#' || (*buffer == '{' && *(buffer + 1) == '-')) {
        if (*buffer == '#') {
            while (*buffer && *buffer != '\n') {
                buffer++;
            }
        } else if (*buffer == '{' && *(buffer + 1) == '-') {
            buffer += 2;
            while (*buffer && !(*buffer == '-' && *(buffer + 1) == '}')) {
                if (*buffer == '\n') {
                    contaLinha++;
                }
                buffer++;
            }
            if (*buffer == '-' && *(buffer + 1) == '}') {
                buffer += 2;
            }
        } else {
            if (*buffer == '\n') {
                contaLinha++;
            }
            buffer++;
        }
    }
}

// Obtém o próximo átomo léxico do código fonte
TInfoAtomo obter_atomo() {
    TInfoAtomo info_atomo;
    ignora_espacos_e_comentarios();
    info_atomo.linha = contaLinha;

    if (islower(*buffer)) {
        info_atomo = reconhece_identificador_ou_palavra_reservada();
    } else if (*buffer == '0' && *(buffer + 1) == 'b') {
        info_atomo = reconhece_numero();
    } else if (*buffer == '(') {
        info_atomo.atomo = ABRE_PAR;
        buffer++;
    } else if (*buffer == ')') {
        info_atomo.atomo = FECHA_PAR;
        buffer++;
    } else if (*buffer == ';') {
        info_atomo.atomo = PONTO_VIRGULA;
        buffer++;
    } else if (*buffer == ',') {
        info_atomo.atomo = VIRGULA;
        buffer++;
    } else if (*buffer == '.') {
        info_atomo.atomo = PONTO;
        buffer++;
    } else if (*buffer == ':') {
        info_atomo.atomo = DOIS_PONTOS;
        buffer++;
    } else if (*buffer == '<') {
        buffer++;
        if (*buffer == '=') {
            info_atomo.atomo = MENOR_IGUAL;
            buffer++;
        } else {
            info_atomo.atomo = MENOR;
        }
    } else if (*buffer == '>') {
        buffer++;
        if (*buffer == '=') {
            info_atomo.atomo = MAIOR_IGUAL;
            buffer++;
        } else {
            info_atomo.atomo = MAIOR;
        }
    } else if (*buffer == '=') {
        info_atomo.atomo = IGUAL;
        buffer++;
    } else if (*buffer == '/' && *(buffer + 1) == '=') {
        info_atomo.atomo = DIFERENTE;
        buffer += 2;
    } else if (*buffer == '+') {
        info_atomo.atomo = MAIS;
        buffer++;
    } else if (*buffer == '-') {
        info_atomo.atomo = MENOS;
        buffer++;
    } else if (*buffer == '*') {
        info_atomo.atomo = MULT;
        buffer++;
    } else if (*buffer == '/') {
        info_atomo.atomo = DIVISAO;
        buffer++;
    } else if (*buffer == '\0') {
        info_atomo.atomo = EOS;
    } else {
        info_atomo.atomo = ERRO;
        buffer++;
    }

    return info_atomo;
}

// Reconhece identificadores e palavras reservadas
TInfoAtomo reconhece_identificador_ou_palavra_reservada() {
    TInfoAtomo info_atomo;
    info_atomo.linha = contaLinha;
    char *inicio = buffer;
    int tamanho = 0;

    while (islower(*buffer) || isdigit(*buffer) || *buffer == '_') {
        buffer++;
        tamanho++;
    }

    strncpy(info_atomo.atributo_ID, inicio, tamanho);
    info_atomo.atributo_ID[tamanho] = '\0';

    if (strcmp(info_atomo.atributo_ID, "program") == 0) {
        info_atomo.atomo = PROGRAM;
    } else if (strcmp(info_atomo.atributo_ID, "begin") == 0) {
        info_atomo.atomo = BEGIN;
    } else if (strcmp(info_atomo.atributo_ID, "end") == 0) {
        info_atomo.atomo = END;
    } else if (strcmp(info_atomo.atributo_ID, "if") == 0) {
        info_atomo.atomo = IF;
    } else if (strcmp(info_atomo.atributo_ID, "elif") == 0) {
        info_atomo.atomo = ELIF;
    } else if (strcmp(info_atomo.atributo_ID, "for") == 0) {
        info_atomo.atomo = FOR;
    } else if (strcmp(info_atomo.atributo_ID, "read") == 0) {
        info_atomo.atomo = READ;
    } else if (strcmp(info_atomo.atributo_ID, "write") == 0) {
        info_atomo.atomo = WRITE;
    } else if (strcmp(info_atomo.atributo_ID, "set") == 0) {
        info_atomo.atomo = SET;
    } else if (strcmp(info_atomo.atributo_ID, "to") == 0) {
        info_atomo.atomo = TO;
    } else if (strcmp(info_atomo.atributo_ID, "of") == 0) {
        info_atomo.atomo = OF;
    } else if (strcmp(info_atomo.atributo_ID, "integer") == 0) {
        info_atomo.atomo = INTEGER;
    } else if (strcmp(info_atomo.atributo_ID, "boolean") == 0) {
        info_atomo.atomo = BOOLEAN;
    } else if (strcmp(info_atomo.atributo_ID, "and") == 0) {
        info_atomo.atomo = AND;
    } else if (strcmp(info_atomo.atributo_ID, "or") == 0) {
        info_atomo.atomo = OR;
    } else if (strcmp(info_atomo.atributo_ID, "not") == 0) {
        info_atomo.atomo = NOT;
    } else if (strcmp(info_atomo.atributo_ID, "true") == 0) {
        info_atomo.atomo = TRUE;
    } else if (strcmp(info_atomo.atributo_ID, "false") == 0) {
        info_atomo.atomo = FALSE;
    } else {
        info_atomo.atomo = IDENTIFICADOR;
    }

    return info_atomo;
}

// Reconhece números binários iniciados por 0b
TInfoAtomo reconhece_numero() {
    TInfoAtomo info_atomo;
    info_atomo.linha = contaLinha;
    buffer += 2;
    int valor = 0;

    if (*buffer != '0' && *buffer != '1') {
        info_atomo.atomo = ERRO;
        return info_atomo;
    }

    while (*buffer == '0' || *buffer == '1') {
        valor = (valor << 1) + (*buffer - '0');
        buffer++;
    }

    info_atomo.atomo = NUMERO;
    info_atomo.atributo_numero = valor;
    return info_atomo;
}

// Reporta erro sintático e encerra o programa
void erro_sintatico(const char *esperado) {
    printf("%03d# erro sintatico, esperado [%s], encontrado [%s]\n", 
           token.linha, esperado, msgAtomo[token.atomo]);
    exit(1);
}

// Reporta erro semântico e encerra o programa
void erro_semantico(const char *mensagem) {
    printf("%03d# erro semantico: %s\n", token.linha, mensagem);
    exit(1);
}

// Consome o átomo atual se for o esperado, senão gera erro
void consome(TAtomo esperado) {
    if (token.atomo == esperado) {
        token = obter_atomo();
    } else {
        erro_sintatico(msgAtomo[esperado]);
    }
}

// Inicia análise sintática do programa
void analisador_sintatico() {
    programa();
}

// Analisa a estrutura principal do programa
void programa() {
    consome(PROGRAM);
    consome(IDENTIFICADOR);
    consome(PONTO_VIRGULA);
    bloco();
    consome(PONTO);
}

// Analisa o bloco de código com declarações e comandos
void bloco() {
    if (token.atomo == INTEGER || token.atomo == BOOLEAN) {
        declaracao_de_variaveis();
    }
    printf("\tAMEM %d\n", conta_simbolos);
    comando_composto();
}

// Analisa declarações de variáveis
void declaracao_de_variaveis() {
    tipo();
    lista_variavel();
    consome(PONTO_VIRGULA);
    while (token.atomo == INTEGER || token.atomo == BOOLEAN) {
        tipo();
        lista_variavel();
        consome(PONTO_VIRGULA);
    }
}

// Analisa tipos de dados
void tipo() {
    if (token.atomo == INTEGER) {
        consome(INTEGER);
    } else if (token.atomo == BOOLEAN) {
        consome(BOOLEAN);
    } else {
        erro_sintatico("tipo");
    }
}

// Analisa lista de variáveis separadas por vírgula
void lista_variavel() {
    if (token.atomo == IDENTIFICADOR) {
        if (busca_tabela_simbolos(token.atributo_ID) == -1) {
            insere_tabela_simbolos(token.atributo_ID);
        } else {
            erro_semantico("Identificador duplicado na declaracao");
        }
        consome(IDENTIFICADOR);
        while (token.atomo == VIRGULA) {
            consome(VIRGULA);
            if (token.atomo == IDENTIFICADOR) {
                if (busca_tabela_simbolos(token.atributo_ID) == -1) {
                    insere_tabela_simbolos(token.atributo_ID);
                } else {
                    erro_semantico("Identificador duplicado na declaracao");
                }
                consome(IDENTIFICADOR);
            } else {
                erro_sintatico("IDENTIFICADOR");
            }
        }
    } else {
        erro_sintatico("IDENTIFICADOR");
    }
}

// Analisa bloco de comandos entre begin e end
void comando_composto() {
    if (token.atomo == BEGIN) {
        consome(BEGIN);
        while (token.atomo != END) {
            comando();
            if (token.atomo == PONTO_VIRGULA) {
                consome(PONTO_VIRGULA);
            } else if (token.atomo != END) {
                erro_sintatico("'end' ou ';'");
                token = obter_atomo();
            }
        }
        consome(END);
    } else {
        erro_sintatico("'begin'");
    }
}

// Analisa comandos individuais
void comando() {
    switch (token.atomo) {
        case IDENTIFICADOR:
        case SET:
            comando_atribuicao();
            break;
        case IF:
            comando_condicional();
            break;
        case FOR:
            comando_repeticao();
            break;
        case READ:
            comando_entrada();
            break;
        case WRITE:
            comando_saida();
            break;
        case BEGIN:
            comando_composto();
            break;
        default:
            erro_sintatico("comando");
    }
}

// Analisa comandos de atribuição
void comando_atribuicao() {
    if (token.atomo == SET) {
        consome(SET);
        if (token.atomo == IDENTIFICADOR) {
            int endereco = busca_tabela_simbolos(token.atributo_ID);
            if (endereco == -1) {
                erro_semantico("Variavel nao declarada");
            }
            consome(IDENTIFICADOR);
            consome(TO);
            expressao();
            printf("\tARMZ %d\n", endereco);
        } else {
            erro_sintatico("IDENTIFICADOR");
        }
    } else if (token.atomo == IDENTIFICADOR) {
        int endereco = busca_tabela_simbolos(token.atributo_ID);
        if (endereco == -1) {
            erro_semantico("Variavel nao declarada");
        }
        consome(IDENTIFICADOR);
        consome(IGUAL);
        expressao();
        printf("\tARMZ %d\n", endereco);
    } else {
        erro_sintatico("Comando de atribuicao");
    }
}

// Analisa estruturas condicionais if-elif
void comando_condicional() {
    int L1 = proximo_rotulo();
    int L2 = proximo_rotulo();
    consome(IF);
    expressao();
    consome(DOIS_PONTOS);
    printf("\tDSVF L%d\n", L1);
    comando();
    printf("\tDSVS L%d\n", L2);
    printf("L%d:\tNADA\n", L1);
    if (token.atomo == ELIF) {
        consome(ELIF);
        comando();
    }
    printf("L%d:\tNADA\n", L2);
}

// Analisa estruturas de repetição for
void comando_repeticao() {
    int L1 = proximo_rotulo();
    int L2 = proximo_rotulo();
    consome(FOR);
    if (token.atomo == IDENTIFICADOR) {
        int endereco_contador = busca_tabela_simbolos(token.atributo_ID);
        if (endereco_contador == -1) {
            erro_semantico("Variavel nao declarada");
        }
        consome(IDENTIFICADOR);
        consome(OF);
        expressao();
        printf("\tARMZ %d\n", endereco_contador);
        consome(TO);
        
        printf("L%d:\tNADA\n", L1);
        printf("\tCRVL %d\n", endereco_contador);
        
        if (token.atomo == NUMERO) {
            printf("\tCRCT %d\n", token.atributo_numero);
            consome(NUMERO);
        } else if (token.atomo == IDENTIFICADOR) {
            int endereco_limite = busca_tabela_simbolos(token.atributo_ID);
            if (endereco_limite == -1) {
                erro_semantico("Variavel nao declarada");
            }
            printf("\tCRVL %d\n", endereco_limite);
            consome(IDENTIFICADOR);
        }
        
        printf("\tCMEG\n");
        printf("\tDSVF L%d\n", L2);
        consome(DOIS_PONTOS);
        
        if (token.atomo == BEGIN) {
            comando_composto();
        } else {
            comando();
        }
        
        printf("\tCRVL %d\n", endereco_contador);
        printf("\tCRCT 1\n");
        printf("\tSOMA\n");
        printf("\tARMZ %d\n", endereco_contador);
        printf("\tDSVS L%d\n", L1);
        printf("L%d:\tNADA\n", L2);
    }
}

// Analisa comandos de entrada de dados
void comando_entrada() {
    consome(READ);
    consome(ABRE_PAR);
    if (token.atomo == IDENTIFICADOR) {
        do {
            int endereco = busca_tabela_simbolos(token.atributo_ID);
            if (endereco == -1) {
                erro_semantico("Variavel nao declarada");
            }
            consome(IDENTIFICADOR);
            printf("\tLEIT\n");
            printf("\tARMZ %d\n", endereco);
            if (token.atomo == VIRGULA) {
                consome(VIRGULA);
            } else {
                break;
            }
        } while (token.atomo == IDENTIFICADOR);
    } else {
        erro_sintatico("IDENTIFICADOR");
    }
    consome(FECHA_PAR);
}

// Analisa comandos de saída de dados
void comando_saida() {
    consome(WRITE);
    consome(ABRE_PAR);
    do {
        expressao();
        printf("\tIMPR\n");
        if (token.atomo == VIRGULA) {
            consome(VIRGULA);
        } else {
            break;
        }
    } while (1);
    consome(FECHA_PAR);
}

// Analisa expressões lógicas e aritméticas
void expressao() {
    expressao_relacional();
    while (token.atomo == AND || token.atomo == OR) {
        TAtomo operador = token.atomo;
        consome(token.atomo);
        expressao_relacional();
        if (operador == AND) {
            printf("\tCONJ\n"); // Instrução MEPA para conjunção (AND)
        } else {
            printf("\tDISJ\n"); // Instrução MEPA para disjunção (OR)
        }
    }
}

// Analisa expressões aritméticas simples
void expressao_simples() {
    termo();
    while (token.atomo == MAIS || token.atomo == MENOS) {
        TAtomo operador = token.atomo;
        consome(token.atomo);
        termo();
        if (operador == MAIS) {
            printf("\tSOMA\n");
        } else {
            printf("\tSUBT\n");
        }
    }
}

// Analisa expressões com operadores relacionais
void expressao_relacional() {
    expressao_simples();
    if (token.atomo == MAIOR || token.atomo == MENOR || token.atomo == IGUAL ||
        token.atomo == DIFERENTE || token.atomo == MAIOR_IGUAL || token.atomo == MENOR_IGUAL) {
        
        TAtomo operador = token.atomo;
        consome(token.atomo);
        expressao_simples();
        
        switch (operador) {
            case MAIOR:
                printf("\tCMMA\n"); // Maior que
                break;
            case MENOR:
                printf("\tCMME\n"); // Menor que
                break;
            case IGUAL:
                printf("\tCMIG\n"); // Igual a
                break;
            case DIFERENTE:
                printf("\tCMDG\n"); // Diferente de
                break;
            case MAIOR_IGUAL:
                printf("\tCMAG\n"); // Maior ou igual a
                break;
            case MENOR_IGUAL:
                printf("\tCMEG\n"); // Menor ou igual a
                break;
            default:
                // Tratamento para valores não esperados
                printf("Erro: Operador relacional desconhecido na linha %d.\n", token.linha);
                exit(1);
        }
    }
}

// Analisa termos com multiplicação e divisão
void termo() {
    fator();
    while (token.atomo == MULT || token.atomo == DIVISAO) {
        TAtomo operador = token.atomo;
        consome(token.atomo);
        fator();
        if (operador == MULT) {
            printf("\tMULT\n");
        } else {
            printf("\tDIVI\n");
        }
    }
}

// Analisa fatores (números, identificadores, expressões entre parênteses)
void fator() {
    if (token.atomo == IDENTIFICADOR) {
        int endereco = busca_tabela_simbolos(token.atributo_ID);
        if (endereco == -1) {
            erro_semantico("Variavel nao declarada");
        }
        printf("\tCRVL %d\n", endereco);
        consome(IDENTIFICADOR);
    } else if (token.atomo == NUMERO) {
        printf("\tCRCT %d\n", token.atributo_numero);
        consome(NUMERO);
    } else if (token.atomo == ABRE_PAR) {
        consome(ABRE_PAR);
        expressao();
        consome(FECHA_PAR);
    } else {
        erro_sintatico("fator");
    }
}

// Busca símbolo na tabela de símbolos
int busca_tabela_simbolos(char *identificador) {
    for (int i = 0; i < conta_simbolos; i++) {
        if (strcmp(tabela_simbolos[i].identificador, identificador) == 0) {
            return tabela_simbolos[i].endereco;
        }
    }
    return -1;
}

// Insere novo símbolo na tabela de símbolos
void insere_tabela_simbolos(char *identificador) {
    if (conta_simbolos >= MAX_SIMBOLOS) {
        printf("Tabela de simbolos cheia\n");
        exit(1);
    }
    strcpy(tabela_simbolos[conta_simbolos].identificador, identificador);
    tabela_simbolos[conta_simbolos].endereco = conta_simbolos;
    conta_simbolos++;
}

// Gera próximo rótulo para desvios
int proximo_rotulo() {
    return contador_rotulos++;
}