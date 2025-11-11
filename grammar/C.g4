grammar C;

prog
    : (expr ';')* EOF
;

expr:   op=('-'|'+') expr                                   # UnaryExpr
    |   expr op=('*'|'/') expr                              # MulDivExpr
    |   expr op=('+'|'-') expr                              # AddSubExpr
    |   expr op=('=='|'!=') expr                            # EqExpr
    |   expr op=('<'|'>'|'<='|'>=') expr                    # RelExpr
    |   ID                                                  # VarRef
    |   expr '=' expr                                       # AssignExpr
    |   INT                                                 # IntLiteral
    |   '(' expr ')'                                        # ParenExpr
    ;


ID          : [a-zA-Z_][a-zA-Z0-9_]* ;
INT         : [0-9]+ ;
NEWLINE     : [\r\n]+ -> skip ;
WS          : [ \t]+  -> skip ;
