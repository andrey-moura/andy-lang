# The Andy Language specification

## Table of Contents
* [1.0.0 - The Lexer](#100---the-lexer)
  * [1.1.0 - Token Types](#110---token-types)
    * [1.1.1 - EOF (End of File)](#111---eof-end-of-file)
    * [1.1.2 - Comments](#112---comments)
    * [1.1.3 - Delimiters](#113---delimiters)
    * [1.1.4 - Numeric Literals](#114---numeric-literals)
* [2.0.0 - Identifying function call](#200---identifying-function-call)
    * [2.1.0 - Without parentheses](#210---without-parentheses)
    * [2.1.1 - With arguments](#211---with-arguments)
    * [2.2.0 - With no arguments](#220---with-no-arguments)
* [3.0.0 - The new keyword](#300---the-new-keyword)

## 1.0.0 - The Lexer

The lexer is responsible for breaking the source code into tokens. It reads the source code character by character and generates tokens based on the rules defined in the lexer specification. The lexer will ignore whitespace, and it will generate tokens for keywords, identifiers, literals, operators and comments.

### 1.1.0 - Token Types

The lexer follows the rules defined below, in the order in which they are presented. A token is emitted as soon as one of the rules is satisfied. The rules are:

- **Literals**: Strings (enclosed in single or double quotes), numbers (integers and floats), booleans (`true` and `false`).
- **Operators**: `+`, `-`, `*`, `/`, `%`, `=`, `==`, `!=`, `<`, `>`, `<=`, `>=`, etc.
- **Delimiters**: `end`, `(`, `)`, `{`, `}`, `[`, `]`, `,`, `;`, etc.

#### 1.1.1 - EOF (End of File)

When the lexer reaches the end of the source code, it will generate a token of type `eof` (end of file). This token is used by the parser to know when to stop parsing.

#### 1.1.2 - Comments

If the lexer finds a double slash `//`, it will generate a token of type `comment` and puts everything (including the `//`) until the end of the line as the content of the token.

#### 1.1.3 - Delimiters

If the lexer finds a delimiter, it will generate a token of type `delimiter` with the content of the delimiter. The possible delimiters are:

- **Delimiters**: `end`, `(`, `)`, `{`, `}`, `[`, `]`, `,`, `;`, etc.

#### 1.1.4 - Numeric Literals

If the lexer finds a numeric literal, it will generate a token of type `number`. The numeric literal can be an integer or a float. The rules are:

1. It can start with an optional sign (`+` or `-`). Note that when the sign is provided, we can have expressions like `1-1`, which is not a -1 token, but a `1` (literal) token followed by a `-` (operator) token and then another `1` (literal) token.
2. The integer part can be one or more digits (`0-9`).
3. It can have an optional decimal point followed by one or more digits (`0-9`), which makes it a double literal.
4. It can end with 'f' to indicate that it is a float literal.

TODO: Write others token types, like identifiers, keywords, etc.

## 2.0.0 - Identifying function call

If the parser finds an expression like `puts('Hello World')`, it will try to identify if it is a function call. The rules are:

1. It should be an identifier (e.g., `puts`).
2. It should be followed by an opening parenthesis `(`.
3. It can have a comma-separated list of arguments, which can be any expression.
4. It should have a closing parenthesis `)`.

### 2.1.0 - Without parentheses

#### 2.1.1 - With arguments

When we find a expression like `puts 'Hello World'`, the parser will try to identify if it is a function call. It may look simple, but it is not. The parser needs to observe the context of the code to determine if it is a function call or not when it reads a token. The rules are:

1. It should be an identifier (e.g., `puts`).
2. It should be followed by a literal or an identifier on the same line.
3. It can have multiple arguments separated by commas.

Examples:

<pre style="background: #1e1e1e; color: #d4d4d4; padding: 1em; font-family: 'Fira Code', monospace; border-radius: 8px;">
    <span style="color: #DCDCAA;">out</span> <span style="color: #CE9178;">'Hello World'</span> <span style="color: #6A9955;">// Calls the function out with one argument</span>
    <span style="color: #DCDCAA;">add</span> <span style="color: #CE9178;">1</span>, <span style="color: #CE9178;">2</span><span style="color: #6A9955;"> // Calls the function add with two arguments</span>
    <span style="color: #DCDCAA;">out</span> <span style="color: #9CDCFE;">variable</span> <span style="color: #6A9955;">// Calls the function out with one argument</span>
    <span style="color: #9CDCFE;">person</span>.<span style="color: #DCDCAA;">say</span><span style="color: #CE9178;"> 'Hello World'</span> <span style="color: #6A9955;">// Calls the instance function say with one argument</span>
</pre>

### 2.2.0 - With no arguments

This can only be identified at runtime. When the interpreter tries to find a variable by it's name (an identifier) in the current context and no matching variable is found, it will try to find a function with the same name in the current context. If a function is found, it will be called with no arguments. Examples:

<pre style="background: #1e1e1e; color: #d4d4d4; padding: 1em; font-family: 'Fira Code', monospace; border-radius: 8px;">
    <span style="color: #DCDCAA;">out</span> <span style="color: #DCDCAA;">getc</span> <span style="color: #6A9955;">// Calls the function getc with no arguments</span>
    <span style="color: #C586C0;">return</span> <span style="color: #DCDCAA;">getc</span> <span style="color: #6A9955;">// Calls the function getc with no arguments</span>
</pre>

<p style="margin-top: 1em;">This is also true for instance functions:</p>

<pre style="background: #1e1e1e; color: #d4d4d4; padding: 1em; font-family: 'Fira Code', monospace; border-radius: 8px;">
    <span style="color: #C586C0;">type</span> <span style="color: #4EC9B0;">Foo</span>
        <span style="color: #C586C0;">fn</span> <span style="color: #DCDCAA;">bar</span>
            <span style="color: #DCDCAA;">out</span> <span style="color: #CE9178;">'Hello from Foo#bar'</span>
        <span style="color: #C586C0;">end</span>
    <span style="color: #C586C0;">end</span>

    <span style="color: #C586C0;">var</span> <span style="color: #9CDCFE;">foo</span> <span style="color: #D4D4D4;">=</span> <span style="color: #C586C0;">new</span> <span style="color: #4EC9B0;">Foo</span><span style="color: #D4D4D4;">()</span>
    <span style="color: #DCDCAA;">out</span> <span style="color: #9CDCFE;">foo</span><span style="color: #D4D4D4;">.</span><span style="color: #DCDCAA;">bar</span> <span style="color: #6A9955;">// Calls the instance function Foo#bar with no arguments</span>
</pre>


#### 3.0.0 - The new keyword

Actually, there is no `new` keyword, it is a function call. When you see something like:

<pre style="background: #1e1e1e; color: #d4d4d4; padding: 1em; font-family: 'Fira Code', monospace; border-radius: 8px;">
    <span style="color: #4EC9B0;">Foo</span>.<span style="color: #DCDCAA;">new</span><span style="color: #D4D4D4;">()</span>
</pre>

or

<pre style="background: #1e1e1e; color: #d4d4d4; padding: 1em; font-family: 'Fira Code', monospace; border-radius: 8px;">
    <span style="color: #4EC9B0;">Foo</span>.<span style="color: #DCDCAA;">new</span>
</pre>

The parser e/or interpreter will apply the [rules for function calls](#200---identifying-function-call). If a instance function `new` is found, it will be called with the provided arguments. If no instance function `new` is found, the interpreter will initialize the object with a default constructor.