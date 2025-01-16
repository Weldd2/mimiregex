/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ast.h                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antoinemura <antoinemura@student.42.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/15 19:18:57 by antoinemura       #+#    #+#             */
/*   Updated: 2025/01/15 19:23:47 by antoinemura      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef AST_H
# define AST_H

/**
 * @enum t_NodeType
 * @brief Represents the type of a node in the abstract syntax tree (AST) for 
 *        the regex engine.
 * 
 * @var NODE_LITERAL
 * Represents a literal character node.
 * 
 * @var NODE_DOT
 * Represents a dot (.) node, which matches any character.
 * 
 * @var NODE_ALTERNATION
 * Represents an alternation (|) node, which matches either the left or right 
 * child.
 * 
 * @var NODE_CONCAT
 * Represents a concatenation node, which matches the sequence of the left and 
 * right children.
 * 
 * @var NODE_QUANTIFIER
 * Represents a quantifier node, which specifies the number of times the 
 * preceding element should be matched.
 * 
 * @var NODE_CHARCLASS
 * Represents a character class node, which matches any character in a set.
 * 
 * @var NODE_GROUP
 * Represents a group node, which captures a sub-pattern.
 */
typedef enum {
	NODE_LITERAL,
	NODE_DOT,
	NODE_ALTERNATION,
	NODE_CONCAT,
	NODE_QUANTIFIER,
	NODE_CHARCLASS,
	NODE_GROUP
} t_NodeType;

/**
 * @struct AST
 * @brief Represents a node in the abstract syntax tree (AST) for the regex 
 *        engine.
 * 
 * @var AST::type
 * The type of the node, as defined by the NodeType enum.
 * 
 * @var AST::left
 * Pointer to the left child node.
 * 
 * @var AST::right
 * Pointer to the right child node.
 * 
 * @var AST::min
 * Minimum number of times the node should be matched (used for quantifiers).
 * 
 * @var AST::max
 * Maximum number of times the node should be matched (used for quantifiers).
 * 
 * @var AST::chars
 * Pointer to a string of characters, used for storing character classes or
 * literals.
 * 
 * @var AST::group_id
 * Index of the capture group, if applicable.
 */
typedef struct s_AST {
	t_NodeType		type;
	struct s_AST	*left;
	struct s_AST	*right;
	int				min;
	int				max;
	char			*chars;
	int				group_id;
} t_AST;

#endif