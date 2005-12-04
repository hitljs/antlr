/*
[The "BSD licence"]
Copyright (c) 2005 Terence Parr
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
package org.antlr.test;

import org.antlr.test.unit.TestSuite;

public class TestSemanticPredicateEvaluation extends TestSuite {
	public void testSimpleCyclicDFAWithPredicate() throws Exception {
		String grammar =
			"grammar foo;\n" +
			"a : {false}? \"x\"* \"y\" {System.out.println(\"alt1\");}\n" +
			"  | {true}?  \"x\"* \"y\" {System.out.println(\"alt2\");}\n" +
			"  ;\n" ;
		String found =
			TestCompileAndExecSupport.execParser("foo.g", grammar, "foo", "fooLexer",
												 "a", "xxxy", false);
		String expecting = "alt2\n";
		assertEqual(found, expecting);
	}

	public void testSimpleCyclicDFAWithInstanceVarPredicate() throws Exception {
		String grammar =
			"grammar foo;\n" +
			"@members {boolean v=true;}\n" +
			"a : {false}? \"x\"* \"y\" {System.out.println(\"alt1\");}\n" +
			"  | {v}?     \"x\"* \"y\" {System.out.println(\"alt2\");}\n" +
			"  ;\n" ;
		String found =
			TestCompileAndExecSupport.execParser("foo.g", grammar, "foo", "fooLexer",
												 "a", "xxxy", false);
		String expecting = "alt2\n";
		assertEqual(found, expecting);
	}

	public void testPredicateValidation() throws Exception {
		String grammar =
			"grammar foo;\n" +
			"@members {\n" +
			"public void reportError(RecognitionException e) {\n" +
			"    System.out.println(\"error: \"+e.toString());\n" +
			"}\n" +
			"}\n" +
			"\n" +
			"a : {false}? \"x\"\n" +
			"  ;\n" ;
		String found =
			TestCompileAndExecSupport.execParser("foo.g", grammar, "foo", "fooLexer",
												 "a", "x", false);
		String expecting = "error: FailedPredicateException(a,{false}?)\n";
		assertEqual(found, expecting);
	}

	public void testLexerPreds() throws Exception {
		String grammar =
			"grammar foo;" +
			"@lexer::members {boolean p=false;}\n" +
			"a : (A|B)+ ;\n" +
			"A : {p}? \"a\"  {System.out.println(\"token 1\");} ;\n" +
			"B : {!p}? \"a\" {System.out.println(\"token 2\");} ;\n";
		String found =
			TestCompileAndExecSupport.execParser("foo.g", grammar, "foo", "fooLexer",
												 "a", "a", false);
		// "a" is ambig; can match both A, B.  Pred says match 2
		String expecting = "token 2\n";
		assertEqual(found, expecting);
	}

	public void testLexerPreds2() throws Exception {
		String grammar =
			"grammar foo;" +
			"@lexer::members {boolean p=true;}\n" +
			"a : (A|B)+ ;\n" +
			"A : {p}? \"a\" {System.out.println(\"token 1\");} ;\n" +
			"B : (\"a\"|\"b\")+ {System.out.println(\"token 2\");} ;\n";
		String found =
			TestCompileAndExecSupport.execParser("foo.g", grammar, "foo", "fooLexer",
												 "a", "a", false);
		// "a" is ambig; can match both A, B.  Pred says match 1
		String expecting = "token 1\n";
		assertEqual(found, expecting);
	}

	public void testLexerPredInExitBranch() throws Exception {
		// p says it's ok to exit; it has precendence over the !p loopback branch
		String grammar =
			"grammar foo;" +
			"@lexer::members {boolean p=true;}\n" +
			"a : (A|B)+ ;\n" +
			"A : (\"a\" {System.out.print(\"1\");})*\n" +
			"    {p}?\n" +
			"    (\"a\" {System.out.print(\"2\");})* ;\n";
		String found =
			TestCompileAndExecSupport.execParser("foo.g", grammar, "foo", "fooLexer",
												 "a", "aaa", false);
		String expecting = "222\n";
		assertEqual(found, expecting);
	}

	public void testLexerPredInExitBranch2() throws Exception {
		String grammar =
			"grammar foo;" +
			"@lexer::members {boolean p=true;}\n" +
			"a : (A|B)+ ;\n" +
			"A : ({p}? \"a\" {System.out.print(\"1\");})*\n" +
			"    (\"a\" {System.out.print(\"2\");})* ;\n";
		String found =
			TestCompileAndExecSupport.execParser("foo.g", grammar, "foo", "fooLexer",
												 "a", "aaa", false);
		String expecting = "111\n";
		assertEqual(found, expecting);
	}

	public void testLexerPredInExitBranch3() throws Exception {
		String grammar =
			"grammar foo;" +
			"@lexer::members {boolean p=true;}\n" +
			"a : (A|B)+ ;\n" +
			"A : ({p}? \"a\" {System.out.print(\"1\");} | )\n" +
			"    (\"a\" {System.out.print(\"2\");})* ;\n";
		String found =
			TestCompileAndExecSupport.execParser("foo.g", grammar, "foo", "fooLexer",
												 "a", "aaa", false);
		String expecting = "122\n";
		assertEqual(found, expecting);
	}

	public void testLexerPredInExitBranch4() throws Exception {
		String grammar =
			"grammar foo;" +
			"a : (A|B)+ ;\n" +
			"A @init {int n=0;} : ({n<2}? \"a\" {System.out.print(n++);})+\n" +
			"    (\"a\" {System.out.print(\"x\");})* ;\n";
		String found =
			TestCompileAndExecSupport.execParser("foo.g", grammar, "foo", "fooLexer",
												 "a", "aaaaa", false);
		String expecting = "01xxx\n";
		assertEqual(found, expecting);
	}

	public void testLexerPredsInCyclicDFA() throws Exception {
		String grammar =
			"grammar foo;" +
			"@lexer::members {boolean p=false;}\n" +
			"a : (A|B)+ ;\n" +
			"A : {p}? (\"a\")+ \"x\"  {System.out.println(\"token 1\");} ;\n" +
			"B :      (\"a\")+ \"x\" {System.out.println(\"token 2\");} ;\n";
		String found =
			TestCompileAndExecSupport.execParser("foo.g", grammar, "foo", "fooLexer",
												 "a", "aax", false);
		String expecting = "token 2\n";
		assertEqual(found, expecting);
	}

	public void testLexerPredsInCyclicDFA2() throws Exception {
		String grammar =
			"grammar foo;" +
			"@lexer::members {boolean p=false;}\n" +
			"a : (A|B)+ ;\n" +
			"A : {p}? (\"a\")+ \"x\" (\"y\")? {System.out.println(\"token 1\");} ;\n" +
			"B :      (\"a\")+ \"x\" {System.out.println(\"token 2\");} ;\n";
		String found =
			TestCompileAndExecSupport.execParser("foo.g", grammar, "foo", "fooLexer",
												 "a", "aax", false);
		String expecting = "token 2\n";
		assertEqual(found, expecting);
	}

	public void testGatedPred() throws Exception {
		String grammar =
			"grammar foo;" +
			"a : (A|B)+ ;\n" +
			"A : {true}?=> \"a\" {System.out.println(\"token 1\");} ;\n" +
			"B : {false}?=>(\"a\"|\"b\")+ {System.out.println(\"token 2\");} ;\n";
		String found =
			TestCompileAndExecSupport.execParser("foo.g", grammar, "foo", "fooLexer",
												 "a", "aa", false);
		// "a" is ambig; can match both A, B.  Pred says match A twice
		String expecting = "token 1\ntoken 1\n";
		assertEqual(found, expecting);
	}

	// S U P P O R T

	public void _test() throws Exception {
		String grammar =
			"grammar T;\n" +
			"options {output=AST;}\n" +
			"a :  ;\n" +
			"ID : \"a\"..\"z\"+ ;\n" +
			"INT : \"0\"..\"9\"+;\n" +
			"WS : (\" \"|\"\\n\") {channel=99;} ;\n";
		String found =
			TestCompileAndExecSupport.execParser("t.g", grammar, "T", "TLexer",
												 "a", "abc 34", false);
		String expecting = "\n";
		assertEqual(found, expecting);
	}

}
