/*
 [The "BSD licence"]
 Copyright (c) 2004 Terence Parr
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
import org.antlr.test.unit.FailedAssertionException;
import org.antlr.tool.*;
import org.antlr.analysis.Label;

import java.util.*;

import antlr.Token;

public class TestSymbolDefinitions extends TestSuite {

	static class ErrorQueue implements ANTLRErrorListener {
		List infos = new LinkedList();
		List errors = new LinkedList();
		List warnings = new LinkedList();

		public void info(String msg) {
			infos.add(msg);
		}

		public void error(Message msg) {
			errors.add(msg);
		}

		public void warning(Message msg) {
			warnings.add(msg);
		}

		public void error(ToolMessage msg) {
			errors.add(msg);
		}

		public void warning(AmbiguityWarning msg) {
			warnings.add(msg);
		}
	};

    /** Public default constructor used by TestRig */
    public TestSymbolDefinitions() {
    }

	public void testParserSimpleTokens() throws Exception {
		Grammar g = new Grammar(
				"parser grammar t;\n"+
				"a : A | B;\n" +
				"b : C ;");
		String rules = "a, b";
		String tokenNames = "A, B, C";
		checkSymbols(g, rules, tokenNames);
	}

	public void testParserTokensSection() throws Exception {
		Grammar g = new Grammar(
				"parser grammar t;\n" +
				"tokens {\n" +
				"  C;\n" +
				"  D;" +
				"}\n"+
				"a : A | B;\n" +
				"b : C ;");
		String rules = "a, b";
		String tokenNames = "A, B, C, D";
		checkSymbols(g, rules, tokenNames);
	}

	public void testLexerTokensSection() throws Exception {
		Grammar g = new Grammar(
				"lexer grammar t;\n" +
				"tokens {\n" +
				"  C;\n" +
				"  D;" +
				"}\n"+
				"A : 'a';\n" +
				"C : 'c' ;");
		String rules = "A, C";
		String tokenNames = "A, C, D";
		checkSymbols(g, rules, tokenNames);
	}

	public void testCombinedGrammarLiterals() throws Exception {
		Grammar g = new Grammar(
				"grammar t;\n"+
				"a : \"begin\" b \"end\";\n" +
				"b : C ';' ;\n" +
				"ID : 'a' ;\n" +
				"FOO : \"foo\" ;\n" +  // "foo" is not a token name
				"C : 'c' ;\n");        // nor is 'c'
		String rules = "a, b";
		String tokenNames = "C, FOO, ID, \"begin\", \"end\", ';'";
		checkSymbols(g, rules, tokenNames);
	}

	// T E S T  E R R O R S

	public void testParserStringLiterals() throws Exception {
		ErrorQueue equeue = new ErrorQueue();
		ErrorManager.setErrorListener(equeue);
		Grammar g = new Grammar(
				"parser grammar t;\n"+
				"a : \"begin\" b ;\n" +
				"b : C ;");
		Object expectedArg = "\"begin\"";
		int expectedMsgID = ErrorManager.MSG_LITERAL_NOT_ASSOCIATED_WITH_LEXER_RULE;
		GrammarSemanticsMessage expectedMessage =
			new GrammarSemanticsMessage(expectedMsgID, g, null, expectedArg);
		checkError(equeue, expectedMessage);
	}

	public void testParserCharLiterals() throws Exception {
		ErrorQueue equeue = new ErrorQueue();
		ErrorManager.setErrorListener(equeue);
		Grammar g = new Grammar(
				"parser grammar t;\n"+
				"a : '(' b ;\n" +
				"b : C ;");
		Object expectedArg = "'('";
		int expectedMsgID = ErrorManager.MSG_LITERAL_NOT_ASSOCIATED_WITH_LEXER_RULE;
		GrammarSemanticsMessage expectedMessage =
			new GrammarSemanticsMessage(expectedMsgID, g, null, expectedArg);
		checkError(equeue, expectedMessage);
	}

	public void testStringLiteralInParserTokensSection() throws Exception {
		ErrorQueue equeue = new ErrorQueue();
		ErrorManager.setErrorListener(equeue); // unique listener per thread
		Grammar g = new Grammar(
				"parser grammar t;\n" +
				"tokens {\n" +
				"  B=\"begin\";\n" +
				"}\n"+
				"a : A B;\n" +
				"b : C ;");
		Object expectedArg = "\"begin\"";
		int expectedMsgID = ErrorManager.MSG_LITERAL_NOT_ASSOCIATED_WITH_LEXER_RULE;
		GrammarSemanticsMessage expectedMessage =
			new GrammarSemanticsMessage(expectedMsgID, g, null, expectedArg);
		checkError(equeue, expectedMessage);
	}

	public void testCharLiteralInParserTokensSection() throws Exception {
		ErrorQueue equeue = new ErrorQueue();
		ErrorManager.setErrorListener(equeue); // unique listener per thread
		Grammar g = new Grammar(
				"parser grammar t;\n" +
				"tokens {\n" +
				"  B='(';\n" +
				"}\n"+
				"a : A B;\n" +
				"b : C ;");
		Object expectedArg = "'('";
		int expectedMsgID = ErrorManager.MSG_LITERAL_NOT_ASSOCIATED_WITH_LEXER_RULE;
		GrammarSemanticsMessage expectedMessage =
			new GrammarSemanticsMessage(expectedMsgID, g, null, expectedArg);
		checkError(equeue, expectedMessage);
	}

	public void testCharLiteralInLexerTokensSection() throws Exception {
		ErrorQueue equeue = new ErrorQueue();
		ErrorManager.setErrorListener(equeue); // unique listener per thread
		Grammar g = new Grammar(
				"lexer grammar t;\n" +
				"tokens {\n" +
				"  B='(';\n" +
				"}\n"+
				"ID : 'a';\n");
		Object expectedArg = "'('";
		int expectedMsgID = ErrorManager.MSG_CANNOT_ALIAS_TOKENS_IN_LEXER;
		GrammarSemanticsMessage expectedMessage =
			new GrammarSemanticsMessage(expectedMsgID, g, null, expectedArg);
		checkError(equeue, expectedMessage);
	}

	public void testRuleRedefinition() throws Exception {
		ErrorQueue equeue = new ErrorQueue();
		ErrorManager.setErrorListener(equeue); // unique listener per thread
		Grammar g = new Grammar(
				"parser grammar t;\n"+
				"a : A | B;\n" +
				"a : C ;");

		Object expectedArg = "a";
		int expectedMsgID = ErrorManager.MSG_RULE_REDEFINITION;
		GrammarSemanticsMessage expectedMessage =
			new GrammarSemanticsMessage(expectedMsgID, g, null, expectedArg);
		checkError(equeue, expectedMessage);
	}

	public void testLexerRuleRedefinition() throws Exception {
		ErrorQueue equeue = new ErrorQueue();
		ErrorManager.setErrorListener(equeue); // unique listener per thread
		Grammar g = new Grammar(
				"lexer grammar t;\n"+
				"ID : 'a' ;\n" +
				"ID : 'd' ;");

		Object expectedArg = "ID";
		int expectedMsgID = ErrorManager.MSG_RULE_REDEFINITION;
		GrammarSemanticsMessage expectedMessage =
			new GrammarSemanticsMessage(expectedMsgID, g, null, expectedArg);
		checkError(equeue, expectedMessage);
	}

	public void testCombinedRuleRedefinition() throws Exception {
		ErrorQueue equeue = new ErrorQueue();
		ErrorManager.setErrorListener(equeue); // unique listener per thread
		Grammar g = new Grammar(
				"grammar t;\n"+
				"x : ID ;\n" +
				"ID : 'a' ;\n" +
				"x : ID ID ;");

		Object expectedArg = "x";
		int expectedMsgID = ErrorManager.MSG_RULE_REDEFINITION;
		GrammarSemanticsMessage expectedMessage =
			new GrammarSemanticsMessage(expectedMsgID, g, null, expectedArg);
		checkError(equeue, expectedMessage);
	}

	public void testUndefinedToken() throws Exception {
		ErrorQueue equeue = new ErrorQueue();
		ErrorManager.setErrorListener(equeue); // unique listener per thread
		Grammar g = new Grammar(
				"grammar t;\n"+
				"x : ID ;");

		Object expectedArg = "ID";
		int expectedMsgID = ErrorManager.MSG_NO_TOKEN_DEFINITION;
		GrammarSemanticsMessage expectedMessage =
			new GrammarSemanticsMessage(expectedMsgID, g, null, expectedArg);
		checkError(equeue, expectedMessage);
	}

	public void testUndefinedTokenOkInParser() throws Exception {
		ErrorQueue equeue = new ErrorQueue();
		ErrorManager.setErrorListener(equeue); // unique listener per thread
		Grammar g = new Grammar(
				"parser grammar t;\n"+
				"x : ID ;");
        assertTrue(equeue.errors.size()==0, "should not be an error");
	}

	public void testUndefinedRule() throws Exception {
		ErrorQueue equeue = new ErrorQueue();
		ErrorManager.setErrorListener(equeue); // unique listener per thread
		Grammar g = new Grammar(
				"grammar t;\n"+
				"x : r ;");

		Object expectedArg = "r";
		int expectedMsgID = ErrorManager.MSG_UNDEFINED_RULE_REF;
		GrammarSemanticsMessage expectedMessage =
			new GrammarSemanticsMessage(expectedMsgID, g, null, expectedArg);
		checkError(equeue, expectedMessage);
	}

	protected void checkError(ErrorQueue equeue,
							  GrammarSemanticsMessage expectedMessage)
		throws FailedAssertionException
	{
		System.out.println(equeue.infos);
		System.out.println(equeue.warnings);
		System.out.println(equeue.errors);
		/*
		assertTrue(equeue.errors.size()==n,
				   "number of errors mismatch; expecting "+n+"; found "+
				   equeue.errors.size());
		*/
		Message foundMsg = null;
		for (int i = 0; i < equeue.errors.size(); i++) {
			Message m = (Message)equeue.errors.get(i);
			if (m.msgID==expectedMessage.msgID ) {
				foundMsg = m;
			}
		}
		assertTrue(foundMsg!=null, "error "+expectedMessage.msgID+" expected");
		assertTrue(foundMsg instanceof GrammarSemanticsMessage,
				   "error is not a GrammarSemanticsMessage");
		assertEqual(foundMsg.arg, expectedMessage.arg);
	}

	protected void checkSymbols(Grammar g,
								String rulesStr,
								String tokensStr)
		throws FailedAssertionException
	{
		Set tokens = g.getTokenNames();
		System.out.println("all tokens="+tokens);

		// make sure expected tokens are there
		StringTokenizer st = new StringTokenizer(tokensStr, ", ");
		while ( st.hasMoreTokens() ) {
			String tokenName = st.nextToken();
			assertTrue(g.getTokenType(tokenName)!=Label.INVALID,
					   "token "+tokenName+" expected");
			tokens.remove(tokenName);
		}
		// make sure there are not any others (other than <EOF> etc...)
        for (Iterator iter = tokens.iterator(); iter.hasNext();) {
			String tokenName = (String) iter.next();
			assertTrue(g.getTokenType(tokenName)<Label.MIN_TOKEN_TYPE,
					   "unexpected token name "+tokenName);
		}

		// make sure all expected rules are there
		st = new StringTokenizer(rulesStr, ", ");
		int n = 0;
		while ( st.hasMoreTokens() ) {
			String ruleName = st.nextToken();
			assertTrue(g.getRule(ruleName)!=null, "rule "+ruleName+" expected");
			n++;
		}
		Collection rules = g.getRules();
		System.out.println("rules="+rules);
		// make sure there are no extra rules
		assertTrue(rules.size()==n,
				   "number of rules mismatch; expecting "+n+"; found "+rules.size());
	}

}
