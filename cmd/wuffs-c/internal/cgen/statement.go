// Copyright 2017 The Wuffs Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package cgen

import (
	"fmt"
	"strings"

	a "github.com/google/wuffs/lang/ast"
	t "github.com/google/wuffs/lang/token"
)

// genFilenameLineComments is whether to print "// foo.wuffs:123\n" comments in
// the generated code. This can be useful for debugging, although it is not
// enabled by default as it can lead to many spurious changes in the generated
// C code (due to line numbers changing) when editing Wuffs code.
const genFilenameLineComments = false

func (g *gen) writeStatement(b *buffer, n *a.Node, depth uint32) error {
	if depth > a.MaxBodyDepth {
		return fmt.Errorf("body recursion depth too large")
	}
	depth++

	if n.Kind() == a.KAssert {
		// Assertions only apply at compile-time.
		return nil
	}

	mightIntroduceTemporaries := false
	switch n.Kind() {
	case a.KAssign:
		n := n.AsAssign()
		mightIntroduceTemporaries = n.RHS().Effect().Coroutine()
	case a.KExpr:
		// TODO.
	}
	if mightIntroduceTemporaries {
		// Put n's code into its own block, to restrict the scope of the
		// tPrefix temporary variables. This helps avoid "jump bypasses
		// variable initialization" compiler warnings with the coroutine
		// suspension points.
		b.writes("{\n")
		defer b.writes("}\n")
	}

	if genFilenameLineComments {
		filename, line := n.AsRaw().FilenameLine()
		if i := strings.LastIndexByte(filename, '/'); i >= 0 {
			filename = filename[i+1:]
		}
		if i := strings.LastIndexByte(filename, '\\'); i >= 0 {
			filename = filename[i+1:]
		}
		b.printf("// %s:%d\n", filename, line)
	}

	switch n.Kind() {
	case a.KAssign:
		n := n.AsAssign()
		return g.writeStatementAssign(b, n.LHS(), n.LHS().MType(), n.Operator(), n.RHS(), depth)
	case a.KExpr:
		return g.writeStatementAssign(b, nil, nil, 0, n.AsExpr(), depth)
	case a.KIOBind:
		return g.writeStatementIOBind(b, n.AsIOBind(), depth)
	case a.KIf:
		return g.writeStatementIf(b, n.AsIf(), depth)
	case a.KIterate:
		return g.writeStatementIterate(b, n.AsIterate(), depth)
	case a.KJump:
		return g.writeStatementJump(b, n.AsJump(), depth)
	case a.KRet:
		return g.writeStatementRet(b, n.AsRet(), depth)
	case a.KVar:
		return nil
	case a.KWhile:
		return g.writeStatementWhile(b, n.AsWhile(), depth)
	}
	return fmt.Errorf("unrecognized ast.Kind (%s) for writeStatement", n.Kind())
}

func (g *gen) writeStatementAssign(b *buffer,
	lhsExpr *a.Expr, lTyp *a.TypeExpr,
	op t.ID, rhsExpr *a.Expr, depth uint32) error {

	// TODO: clean this method body up.

	if depth > a.MaxExprDepth {
		return fmt.Errorf("expression recursion depth too large")
	}
	depth++

	if op != 0 && rhsExpr.Effect().Coroutine() {
		if err := g.writeQuestionCall(b, rhsExpr, depth, op == t.IDEqQuestion); err != nil {
			return err
		}
	}

	closer := ""
	if lTyp == nil {
		// No-op.

	} else {
		lhs := buffer(nil)
		if err := g.writeExpr(&lhs, lhsExpr, depth); err != nil {
			return err
		}

		opName := ""
		if lTyp.IsArrayType() {
			b.writes("memcpy(")
			opName, closer = ",", fmt.Sprintf(", sizeof(%s))", lhs)

		} else {
			switch op {
			case t.IDTildeSatPlusEq, t.IDTildeSatMinusEq:
				uBits := uintBits(lTyp.QID())
				if uBits == 0 {
					return fmt.Errorf("unsupported tilde-operator type %q", lTyp.Str(g.tm))
				}
				uOp := "add"
				if op != t.IDTildeSatPlusEq {
					uOp = "sub"
				}
				b.printf("wuffs_base__u%d__sat_%s_indirect(&", uBits, uOp)
				opName, closer = ",", ")"

			default:
				opName = cOpName(op)
				if opName == "" {
					return fmt.Errorf("unrecognized operator %q", op.AmbiguousForm().Str(g.tm))
				}
			}
		}

		b.writex(lhs)
		b.writes(opName)
	}

	if op == 0 {
		if rhsExpr.Effect().Coroutine() {
			if err := g.writeCoroSuspPoint(b, false); err != nil {
				return err
			}
		}

		if err := g.writeBuiltinQuestionCall(b, rhsExpr, depth); err != errNoSuchBuiltin {
			return err
		}

		if err := g.writeSaveExprDerivedVars(b, rhsExpr); err != nil {
			return err
		}

		if rhsExpr.Effect().Optional() {
			b.writes("status = ")
		}

		// TODO: drop the "Other" in writeExprOther.
		if err := g.writeExprOther(b, rhsExpr, depth); err != nil {
			return err
		}
	} else if err := g.writeExpr(b, rhsExpr, depth); err != nil {
		return err
	}

	b.writes(closer)
	b.writes(";\n")

	if op == 0 {
		if err := g.writeLoadExprDerivedVars(b, rhsExpr); err != nil {
			return err
		}

		if rhsExpr.Effect().Optional() {
			target := "exit"
			if rhsExpr.Effect().Coroutine() {
				target = "suspend"
			}
			b.printf("if (status) { goto %s; }\n", target)
		}
	}

	return nil
}

func (g *gen) writeStatementIOBind(b *buffer, n *a.IOBind, depth uint32) error {
	if g.currFunk.ioBinds > maxIOBinds {
		return fmt.Errorf("too many temporary variables required")
	}
	ioBindNum := g.currFunk.ioBinds
	g.currFunk.ioBinds++

	// TODO: do these variables need to be func-scoped (bigger scope)
	// instead of block-scoped (smaller scope) if the coro_susp_point
	// switch can jump past this initialization??
	b.writes("{\n")
	{
		e := n.IO()
		// TODO: restrict (in the type checker or parser) that e is either a
		// local variable or args.foo?
		prefix := vPrefix
		if e.Operator() != 0 {
			prefix = aPrefix
		}
		cTyp := "reader"
		if e.MType().QID()[1] == t.IDIOWriter {
			cTyp = "writer"
		}
		name := e.Ident().Str(g.tm)
		b.printf("wuffs_base__io_%s %s%d_%s%s = %s%s;\n",
			cTyp, oPrefix, ioBindNum, prefix, name, prefix, name)

		// TODO: save / restore all iop vars, not just for local IO vars? How
		// does this work if the io_bind body advances these pointers, either
		// directly or by calling other funcs?
		if e.Operator() == 0 {
			b.printf("uint8_t *%s%d_%s%s%s = %s%s%s;\n",
				oPrefix, ioBindNum, iopPrefix, prefix, name, iopPrefix, prefix, name)
			b.printf("uint8_t *%s%d_%s%s%s = %s%s%s;\n",
				oPrefix, ioBindNum, io1Prefix, prefix, name, io1Prefix, prefix, name)
		}

		if n.Keyword() == t.IDIOBind {
			b.printf("wuffs_base__io_%s__set(&%s%s, &u_%s, &iop_%s%s, &io1_%s%s,",
				cTyp, prefix, name, name, prefix, name, prefix, name)
			if err := g.writeExpr(b, n.Arg1(), 0); err != nil {
				return err
			}
			b.writes(");\n")

		} else {
			// TODO: restrict (in the type checker or parser) that e is
			// args.foo?
			b.printf("wuffs_base__io_%s__set_limit(&%s%s, iop_%s%s,\n", cTyp, prefix, name, prefix, name)
			if err := g.writeExpr(b, n.Arg1(), 0); err != nil {
				return err
			}
			b.writes(");\n")
		}
	}

	for _, o := range n.Body() {
		if err := g.writeStatement(b, o, depth); err != nil {
			return err
		}
	}

	{
		e := n.IO()
		prefix := vPrefix
		if e.Operator() != 0 {
			prefix = aPrefix
		}
		name := e.Ident().Str(g.tm)
		b.printf("%s%s = %s%d_%s%s;\n",
			prefix, name, oPrefix, ioBindNum, prefix, name)
		if e.Operator() == 0 {
			b.printf("%s%s%s = %s%d_%s%s%s;\n",
				iopPrefix, prefix, name, oPrefix, ioBindNum, iopPrefix, prefix, name)
			b.printf("%s%s%s = %s%d_%s%s%s;\n",
				io1Prefix, prefix, name, oPrefix, ioBindNum, io1Prefix, prefix, name)
		}
	}
	b.writes("}\n")
	return nil
}

func (g *gen) writeStatementIf(b *buffer, n *a.If, depth uint32) error {
	for {
		condition := buffer(nil)
		if err := g.writeExpr(&condition, n.Condition(), 0); err != nil {
			return err
		}
		// Calling trimParens avoids clang's -Wparentheses-equality warning.
		b.printf("if (%s) {\n", trimParens(condition))
		for _, o := range n.BodyIfTrue() {
			if err := g.writeStatement(b, o, depth); err != nil {
				return err
			}
		}
		if bif := n.BodyIfFalse(); len(bif) > 0 {
			b.writes("} else {\n")
			for _, o := range bif {
				if err := g.writeStatement(b, o, depth); err != nil {
					return err
				}
			}
			break
		}
		n = n.ElseIf()
		if n == nil {
			break
		}
		b.writes("} else ")
	}
	b.writes("}\n")
	return nil
}

func (g *gen) writeStatementIterate(b *buffer, n *a.Iterate, depth uint32) error {
	assigns := n.Assigns()
	if len(assigns) == 0 {
		return nil
	}
	if len(assigns) != 1 {
		return fmt.Errorf("TODO: iterate over more than one assign")
	}
	o := assigns[0].AsAssign()
	name := o.LHS().Ident().Str(g.tm)
	b.writes("{\n")

	// TODO: don't assume that the slice is a slice of base.u8. In
	// particular, the code gen can be subtle if the slice element type has
	// zero size, such as the empty struct.
	b.printf("wuffs_base__slice_u8 %sslice_%s =", iPrefix, name)
	if err := g.writeExpr(b, o.RHS(), 0); err != nil {
		return err
	}
	b.writes(";\n")
	b.printf("%s%s = %sslice_%s;\n", vPrefix, name, iPrefix, name)
	// TODO: look at n.HasContinue() and n.HasBreak().

	round := uint32(0)
	for ; n != nil; n = n.ElseIterate() {
		length := n.Length().SmallPowerOf2Value()
		unroll := n.Unroll().SmallPowerOf2Value()
		for {
			if err := g.writeIterateRound(b, name, n.Body(), round, depth, length, unroll); err != nil {
				return err
			}
			round++

			if unroll == 1 {
				break
			}
			unroll = 1
		}
	}

	b.writes("}\n")
	return nil
}

func (g *gen) writeStatementJump(b *buffer, n *a.Jump, depth uint32) error {
	jt, err := g.currFunk.jumpTarget(n.JumpTarget())
	if err != nil {
		return err
	}
	keyword := "continue"
	if n.Keyword() == t.IDBreak {
		keyword = "break"
	}
	b.printf("goto label_%d_%s;\n", jt, keyword)
	return nil
}

func (g *gen) writeStatementRet(b *buffer, n *a.Ret, depth uint32) error {
	retExpr := n.Value()

	if g.currFunk.astFunc.Effect().Optional() {
		isError, isOK := false, false
		b.writes("status = ")
		if retExpr.Operator() == 0 && retExpr.Ident() == t.IDOk {
			b.writes("NULL")
			isOK = true
		} else {
			if retExpr.Ident().IsStrLiteral(g.tm) {
				msg, _ := t.Unescape(retExpr.Ident().Str(g.tm))
				isError = statusMsgIsError(msg)
				isOK = statusMsgIsWarning(msg)
			}
			// TODO: check that retExpr has no call-suspendibles.
			if err := g.writeExpr(
				b, retExpr, depth); err != nil {
				return err
			}
		}
		b.writes(";")

		if n.Keyword() == t.IDYield {
			return g.writeCoroSuspPoint(b, true)
		}

		if isError {
			b.writes("goto exit;")
		} else if isOK {
			g.currFunk.hasGotoOK = true
			b.writes("goto ok;")
		} else {
			g.currFunk.hasGotoOK = true
			// TODO: the "goto exit"s can be "goto ok".
			b.writes("if (wuffs_base__status__is_error(status)) { goto exit; }" +
				"else if (wuffs_base__status__is_suspension(status)) { " +
				"status = wuffs_base__error__cannot_return_a_suspension; goto exit; } goto ok;")
		}
		return nil
	}

	b.writes("return ")
	if g.currFunk.astFunc.Out() == nil {
		return fmt.Errorf("TODO: allow empty return type (when not suspendible)")
	} else if err := g.writeExpr(b, retExpr, depth); err != nil {
		return err
	}
	b.writeb(';')
	return nil
}

func (g *gen) writeStatementWhile(b *buffer, n *a.While, depth uint32) error {
	if n.HasContinue() {
		jt, err := g.currFunk.jumpTarget(n)
		if err != nil {
			return err
		}
		b.printf("label_%d_continue:;\n", jt)
	}
	condition := buffer(nil)
	if err := g.writeExpr(&condition, n.Condition(), 0); err != nil {
		return err
	}
	// Calling trimParens avoids clang's -Wparentheses-equality warning.
	b.printf("while (%s) {\n", trimParens(condition))
	for _, o := range n.Body() {
		if err := g.writeStatement(b, o, depth); err != nil {
			return err
		}
	}
	b.writes("}\n")
	if n.HasBreak() {
		jt, err := g.currFunk.jumpTarget(n)
		if err != nil {
			return err
		}
		b.printf("label_%d_break:;\n", jt)
	}
	return nil
}

func (g *gen) writeIterateRound(b *buffer, name string, body []*a.Node, round uint32, depth uint32, length int, unroll int) error {
	b.printf("%s%s.len = %d;\n", vPrefix, name, length)
	b.printf("uint8_t* %send%d_%s = %sslice_%s.ptr + (%sslice_%s.len / %d) * %d;\n",
		iPrefix, round, name, iPrefix, name, iPrefix, name, length*unroll, length*unroll)
	b.printf("while (%s%s.ptr < %send%d_%s) {\n", vPrefix, name, iPrefix, round, name)
	for i := 0; i < unroll; i++ {
		for _, o := range body {
			if err := g.writeStatement(b, o, depth); err != nil {
				return err
			}
		}
		b.printf("%s%s.ptr += %d;\n", vPrefix, name, length)
	}
	b.writes("}\n")
	return nil
}

func (g *gen) writeCoroSuspPoint(b *buffer, maybeSuspend bool) error {
	const maxCoroSuspPoint = 0xFFFFFFFF
	g.currFunk.coroSuspPoint++
	if g.currFunk.coroSuspPoint == maxCoroSuspPoint {
		return fmt.Errorf("too many coroutine suspension points required")
	}

	macro := ""
	if maybeSuspend {
		macro = "_MAYBE_SUSPEND"
	}
	b.printf("WUFFS_BASE__COROUTINE_SUSPENSION_POINT%s(%d);\n", macro, g.currFunk.coroSuspPoint)
	return nil
}

func (g *gen) writeQuestionCall(b *buffer, n *a.Expr, depth uint32, eqQuestion bool) error {
	if depth > a.MaxExprDepth {
		return fmt.Errorf("expression recursion depth too large")
	}
	depth++

	if !eqQuestion && n.Effect().Coroutine() {
		if err := g.writeCoroSuspPoint(b, false); err != nil {
			return err
		}
	}

	if err := g.writeBuiltinQuestionCall(b, n, depth); err != errNoSuchBuiltin {
		return err
	}

	if err := g.writeSaveExprDerivedVars(b, n); err != nil {
		return err
	}

	if eqQuestion {
		if g.currFunk.tempW > maxTemp {
			return fmt.Errorf("too many temporary variables required")
		}
		temp := g.currFunk.tempW
		g.currFunk.tempW++

		b.printf("wuffs_base__status %s%d = ", tPrefix, temp)
	} else {
		b.writes("status = ")
	}

	if err := g.writeExprUserDefinedCall(b, n, depth); err != nil {
		return err
	}
	b.writes(";\n")

	if err := g.writeLoadExprDerivedVars(b, n); err != nil {
		return err
	}

	if !eqQuestion {
		b.writes("if (status) { goto suspend; }\n")
	}
	return nil
}

func trimParens(b []byte) []byte {
	if len(b) > 1 && b[0] == '(' && b[len(b)-1] == ')' {
		return b[1 : len(b)-1]
	}
	return b
}
