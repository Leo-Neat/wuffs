// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

// +build ignore

package main

// extract-flate-offsets.go extracts the start and end offsets of the
// flate-compressed data wrapped in a .gz file.
//
// Usage: go run extract-flate-offsets.go foo.gz bar.gz
//
// Alternatively: go run extract-flate-offsets.go -write-zlib foo.gz

import (
	"bytes"
	"compress/flate"
	"compress/zlib"
	"crypto/md5"
	"flag"
	"fmt"
	"hash/adler32"
	"io/ioutil"
	"os"
	"strings"
)

var (
	writeZlib = flag.Bool("write-zlib", false, "whether to convert gzip to zlib")
)

// GZIP wraps a header and footer around flate data. The format is described in
// RFC 1952: https://www.ietf.org/rfc/rfc1952.txt
const (
	flagText    = 1 << 0
	flagHCRC    = 1 << 1
	flagExtra   = 1 << 2
	flagName    = 1 << 3
	flagComment = 1 << 4
)

func main() {
	if err := main1(); err != nil {
		os.Stderr.WriteString(err.Error() + "\n")
		os.Exit(1)
	}
}

func main1() error {
	flag.Parse()
	for _, a := range flag.Args() {
		if err := decode(a); err != nil {
			return err
		}
	}
	return nil
}

func decode(filename string) error {
	src, err := ioutil.ReadFile(filename)
	if err != nil {
		return err
	}

	const (
		headerSize = 10
		footerSize = 8
	)
	if len(src) < headerSize+footerSize || src[0] != 0x1F || src[1] != 0x8B || src[2] != 0x08 {
		return fmt.Errorf("not a GZIP")
	}
	if len(src) >= 0x10000000 {
		return fmt.Errorf("file too large")
	}
	flags := src[3]
	i := headerSize
	src = src[:len(src)-footerSize]

	if flags&flagExtra != 0 {
		return fmt.Errorf("TODO: support gzip extra flag")
	}

	if flags&flagName != 0 {
		if i, err = readString(src, i); err != nil {
			return err
		}
	}

	if flags&flagComment != 0 {
		if i, err = readString(src, i); err != nil {
			return err
		}
	}

	if flags&flagHCRC != 0 {
		return fmt.Errorf("TODO: support gzip HCRC flag")
	}

	// As a sanity check, the result should be valid flate.
	uncompressed, err := checkFlate(src[i:])
	if err != nil {
		return err
	}

	if *writeZlib {
		return doWriteZlib(src[i:], uncompressed, filename)
	}
	fmt.Printf("%7d %7d %x  %s\n", i, len(src), md5.Sum(uncompressed), filename)
	return nil
}

func doWriteZlib(flateCompressed []byte, uncompressed []byte, filename string) error {
	buf := bytes.NewBuffer(nil)
	// The ZLIB header (as per https://www.ietf.org/rfc/rfc1950.txt) is 2
	// bytes.
	//
	// The first byte's low 4 bits is the compression method: 8 means flate.
	// The first byte's high 4 bits is the compression info: 7 means a 32KiB
	// flate window size.
	//
	// The second byte's low 5 bits are a parity check. The 5th bit (0 in this
	// case) indicates a preset dictionary. The high 2 bits (2 in this case)
	// means the default compression algorithm.
	buf.WriteString("\x78\x9c")
	// Write the payload.
	buf.Write(flateCompressed)
	// The ZLIB footer is 4 bytes: a big-endian checksum.
	checksum := adler32.Checksum(uncompressed)
	buf.WriteByte(uint8(checksum >> 24))
	buf.WriteByte(uint8(checksum >> 16))
	buf.WriteByte(uint8(checksum >> 8))
	buf.WriteByte(uint8(checksum >> 0))

	asZlib := buf.Bytes()

	// As a sanity check, the result should be valid zlib.
	if _, err := checkZlib(asZlib); err != nil {
		return err
	}

	if strings.HasSuffix(filename, ".gz") {
		filename = filename[:len(filename)-3]
	}
	filename += ".zlib"
	if err := ioutil.WriteFile(filename, asZlib, 0666); err != nil {
		return err
	}
	fmt.Printf("wrote %s\n", filename)
	return nil
}

func readString(src []byte, i int) (int, error) {
	for {
		if i >= len(src) {
			return 0, fmt.Errorf("bad GZIP string")
		}
		if src[i] == 0 {
			return i + 1, nil
		}
		i++
	}
}

func checkFlate(x []byte) ([]byte, error) {
	rc := flate.NewReader(bytes.NewReader(x))
	defer rc.Close()
	x, err := ioutil.ReadAll(rc)
	if err != nil {
		return nil, fmt.Errorf("data is not valid flate: %v", err)
	}
	return x, nil
}

func checkZlib(x []byte) ([]byte, error) {
	rc, err := zlib.NewReader(bytes.NewReader(x))
	if err != nil {
		return nil, fmt.Errorf("data is not valid zlib: %v", err)
	}
	defer rc.Close()
	x, err = ioutil.ReadAll(rc)
	if err != nil {
		return nil, fmt.Errorf("data is not valid zlib: %v", err)
	}
	return x, nil
}