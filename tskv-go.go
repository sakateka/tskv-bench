package main

import (
	"bufio"
	"bytes"
	"fmt"
	"log"
	"os"
	"regexp"
)

var tab = []byte("\t")
var equal = []byte("=")

type index struct {
	valueOffset int
	start       int
	end         int
}

type indexer struct {
	initialized  bool
	line         []byte
	fieldIndexes map[string]int
	i            []index
}

func (i *indexer) readLine(reader *bufio.Reader) error {
	i.line = i.line[:0]
	line, isPrefix, err := reader.ReadLine()
	if err != nil {
		return err
	}
	i.line = append(i.line, line...)
	if !isPrefix {
		return nil
	}
	for isPrefix && err == nil {
		line, isPrefix, err = reader.ReadLine()
		if err == nil {
			i.line = append(i.line[:len(i.line)], line...)
		}
	}
	return err

}

func (i *indexer) parseFields() int {
	var (
		cursor  int
		idx     int
		length  int
		nameLen int
	)
	if len(i.line) < 5 || string(i.line[0:5]) != "tskv\t" {
		return 0
	}
	for {
		// tskv\t skiped and i.i[0] is first key=value field
		idx = bytes.Index(i.line[cursor:], tab)
		if idx < 0 {
			break
		}
		cursor += idx + 1
		if !i.initialized {
			i.i = append(i.i, index{})
			nameLen = bytes.Index(i.line[cursor:], equal)
			i.i[length].valueOffset = nameLen + 1
			i.fieldIndexes[string(i.line[cursor:cursor+nameLen])] = length
		}
		i.i[length].start = cursor
		if length > 0 {
			i.i[length-1].end = cursor - 1
		}
		length++
	}
	//if !i.initialized {
	//	for k, v := range i.fieldIndexes {
	//		fmt.Println(k, v)
	//	}
	//}
	i.initialized = true
	i.i[length-1].end = len(i.line)
	return length + 1 // +1 for count leading tskv\t
}

func (i *indexer) v(name string) []byte {
	index := i.i[i.fieldIndexes[name]]
	return i.line[index.start+index.valueOffset : index.end]
}

func main() {
	var (
		length    int
		requestRe = `^/[^/?:.-]{3,6}/`
	)
	re := regexp.MustCompile(requestRe)
	matchedRequests := make(map[string]int)

	lines := make(map[int]int, 1)
	layout := &indexer{
		fieldIndexes: make(map[string]int),
	}

	logFile, err := os.Open("access.log")
	if err != nil {
		log.Fatal(err)
	}
	log := bufio.NewReader(logFile)

	for {
		if err = layout.readLine(log); err != nil {
			break
		}
		length = layout.parseFields()
		if length == 0 {
			continue
		}
		if matched := re.Find(layout.v("request")); matched != nil {
			matchedRequests[string(matched)]++
		}
		lines[length]++
	}
	fmt.Println(lines)
	for key, value := range matchedRequests {
		fmt.Printf("%s %d\n", key, value)
	}
}
