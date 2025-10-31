grammar Expr;

prog:   expr EOF ;

expr:   ('-'|'+') expr
    |   expr ('*'|'/') expr
    |   expr ('+'|'-') expr
    |   INT
    |   '(' expr ')'
    ;

IDENTIFIER: [a-zA-Z_][a-zA-Z0-9_]* ;
INT     : [0-9]+ ;
NEWLINE : [\r\n]+ -> skip;
WS      : [ \t]+ -> skip;
