/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   token.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antoinemura <antoinemura@student.42.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/15 19:18:23 by antoinemura       #+#    #+#             */
/*   Updated: 2025/01/15 19:22:38 by antoinemura      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef TOKEN_H
# define TOKEN_H

typedef enum {
	TOKEN_LITERAL,
	TOKEN_DOT,
	TOKEN_STAR,
	TOKEN_PLUS,
	TOKEN_QUESTION,
	TOKEN_LPAREN,
	TOKEN_RPAREN,
	TOKEN_LBRACKET,
	TOKEN_RBRACKET,
	TOKEN_PIPE,
	TOKEN_ESCAPE,
	TOKEN_EOF
} t_TokenType;

typedef struct {
	const char	*input;
	size_t		pos;
} Lexer;

#endif