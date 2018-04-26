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

package ast

import (
	t "github.com/google/wuffs/lang/token"
)

// Str returns a string form of n.
func (n *Expr) Str(tm *t.Map) string {
	if n == nil {
		return ""
	}
	if n.id0 == 0 && n.id1 == 0 {
		return tm.ByID(n.id2)
	}
	return string(n.appendStr(nil, tm, false, 0))
}

func (n *Expr) appendStr(buf []byte, tm *t.Map, parenthesize bool, depth uint32) []byte {
	if depth > MaxExprDepth {
		return append(buf, "!expr_recursion_depth_too_large!"...)
	}
	depth++

	if n == nil {
		return buf
	}

	if n.id0.IsXOp() {
		switch {
		case n.id0.IsXUnaryOp():
			buf = append(buf, opStrings[0xFF&n.id0]...)
			buf = n.rhs.Expr().appendStr(buf, tm, true, depth)

		case n.id0.IsXBinaryOp():
			if parenthesize {
				buf = append(buf, '(')
			}
			buf = n.lhs.Expr().appendStr(buf, tm, true, depth)
			buf = append(buf, opStrings[0xFF&n.id0]...)
			if n.id0 == t.IDXBinaryAs {
				buf = append(buf, n.rhs.TypeExpr().Str(tm)...)
			} else {
				buf = n.rhs.Expr().appendStr(buf, tm, true, depth)
			}
			if parenthesize {
				buf = append(buf, ')')
			}

		case n.id0.IsXAssociativeOp():
			if parenthesize {
				buf = append(buf, '(')
			}
			op := opStrings[0xFF&n.id0]
			for i, o := range n.list0 {
				if i != 0 {
					buf = append(buf, op...)
				}
				buf = o.Expr().appendStr(buf, tm, true, depth)
			}
			if parenthesize {
				buf = append(buf, ')')
			}
		}

	} else {
		switch n.id0 {
		case t.IDError, t.IDStatus, t.IDSuspension:
			switch n.id0 {
			case t.IDError:
				buf = append(buf, "error "...)
			case t.IDStatus:
				buf = append(buf, "status "...)
			case t.IDSuspension:
				buf = append(buf, "suspension "...)
			}
			if n.id1 != 0 {
				buf = append(buf, tm.ByID(n.id1)...)
				buf = append(buf, '.')
			}
			fallthrough

		case 0:
			buf = append(buf, tm.ByID(n.id2)...)

		case t.IDTry:
			buf = append(buf, "try "...)
			fallthrough

		case t.IDOpenParen:
			buf = n.lhs.Expr().appendStr(buf, tm, true, depth)
			if n.flags&FlagsSuspendible != 0 {
				buf = append(buf, '?')
			} else if n.flags&FlagsImpure != 0 {
				buf = append(buf, '!')
			}
			buf = append(buf, '(')
			for i, o := range n.list0 {
				if i != 0 {
					buf = append(buf, ", "...)
				}
				buf = append(buf, tm.ByID(o.Arg().Name())...)
				buf = append(buf, ':')
				buf = o.Arg().Value().appendStr(buf, tm, false, depth)
			}
			buf = append(buf, ')')

		case t.IDOpenBracket:
			buf = n.lhs.Expr().appendStr(buf, tm, true, depth)
			buf = append(buf, '[')
			buf = n.rhs.Expr().appendStr(buf, tm, false, depth)
			buf = append(buf, ']')

		case t.IDColon:
			buf = n.lhs.Expr().appendStr(buf, tm, true, depth)
			buf = append(buf, '[')
			buf = n.mhs.Expr().appendStr(buf, tm, false, depth)
			buf = append(buf, ':')
			buf = n.rhs.Expr().appendStr(buf, tm, false, depth)
			buf = append(buf, ']')

		case t.IDDot:
			buf = n.lhs.Expr().appendStr(buf, tm, true, depth)
			buf = append(buf, '.')
			buf = append(buf, tm.ByID(n.id2)...)

		case t.IDDollar:
			buf = append(buf, "$("...)
			for i, o := range n.list0 {
				if i != 0 {
					buf = append(buf, ", "...)
				}
				buf = o.Expr().appendStr(buf, tm, false, depth)
			}
			buf = append(buf, ')')
		}
	}

	return buf
}

var opStrings = [256]string{
	t.IDXUnaryPlus:  "+",
	t.IDXUnaryMinus: "-",
	t.IDXUnaryNot:   "not ",
	t.IDXUnaryRef:   "ref ",
	t.IDXUnaryDeref: "deref ",

	t.IDXBinaryPlus:          " + ",
	t.IDXBinaryMinus:         " - ",
	t.IDXBinaryStar:          " * ",
	t.IDXBinarySlash:         " / ",
	t.IDXBinaryShiftL:        " << ",
	t.IDXBinaryShiftR:        " >> ",
	t.IDXBinaryAmp:           " & ",
	t.IDXBinaryAmpHat:        " &^ ",
	t.IDXBinaryPipe:          " | ",
	t.IDXBinaryHat:           " ^ ",
	t.IDXBinaryTildeModPlus:  " ~mod+ ",
	t.IDXBinaryTildeModMinus: " ~mod- ",
	t.IDXBinaryTildeSatPlus:  " ~sat+ ",
	t.IDXBinaryTildeSatMinus: " ~sat- ",
	t.IDXBinaryNotEq:         " != ",
	t.IDXBinaryLessThan:      " < ",
	t.IDXBinaryLessEq:        " <= ",
	t.IDXBinaryEqEq:          " == ",
	t.IDXBinaryGreaterEq:     " >= ",
	t.IDXBinaryGreaterThan:   " > ",
	t.IDXBinaryAnd:           " and ",
	t.IDXBinaryOr:            " or ",
	t.IDXBinaryAs:            " as ",

	t.IDXAssociativePlus: " + ",
	t.IDXAssociativeStar: " * ",
	t.IDXAssociativeAmp:  " & ",
	t.IDXAssociativePipe: " | ",
	t.IDXAssociativeHat:  " ^ ",
	t.IDXAssociativeAnd:  " and ",
	t.IDXAssociativeOr:   " or ",
}

// Str returns a string form of n.
func (n *TypeExpr) Str(tm *t.Map) string {
	if n == nil {
		return ""
	}
	if n.Decorator() == 0 && n.Min() == nil && n.Max() == nil {
		return n.QID().Str(tm)
	}
	return string(n.appendStr(nil, tm, 0))
}

func (n *TypeExpr) appendStr(buf []byte, tm *t.Map, depth uint32) []byte {
	if depth > MaxTypeExprDepth {
		return append(buf, "!type_expr_recursion_depth_too_large!"...)
	}
	depth++
	if n == nil {
		return append(buf, "!invalid_type!"...)
	}

	switch n.Decorator() {
	case 0:
		buf = append(buf, n.QID().Str(tm)...)
	case t.IDPtr:
		buf = append(buf, "ptr "...)
		return n.Inner().appendStr(buf, tm, depth)
	case t.IDArray:
		buf = append(buf, "array["...)
		buf = n.ArrayLength().appendStr(buf, tm, false, 0)
		buf = append(buf, "] "...)
		return n.Inner().appendStr(buf, tm, depth)
	case t.IDSlice:
		buf = append(buf, "slice "...)
		return n.Inner().appendStr(buf, tm, depth)
	case t.IDFunc:
		buf = append(buf, "func "...)
		if r := n.Receiver(); r != nil {
			buf = append(buf, '(')
			buf = r.appendStr(buf, tm, depth)
			buf = append(buf, ")."...)
		}
		return append(buf, n.FuncName().Str(tm)...)
	default:
		return append(buf, "!invalid_type!"...)
	}
	if n.Min() != nil || n.Max() != nil {
		buf = append(buf, '[')
		buf = n.Min().appendStr(buf, tm, false, 0)
		buf = append(buf, ".."...)
		buf = n.Max().appendStr(buf, tm, false, 0)
		buf = append(buf, ']')
	}
	return buf
}
