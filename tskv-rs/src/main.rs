extern crate memchr;
extern crate regex;

use memchr::memchr;
use std::fs::File;
use std::io::{BufRead, BufReader};
use std::collections::HashMap;
use regex::bytes::Regex;

struct Index {
    value_offset: usize,
    end: usize,
}

struct Indexer {
    initialized: bool,
    line: Vec<u8>,
    field_indexes: HashMap<String, usize>,
    i: Vec<Index>,
}

impl Indexer {
    fn parse_fields(&mut self, len: usize) -> usize {
        let mut counter: usize = 0;
        let mut prev_index;
        let mut cursor: usize = 0;
        while cursor < len {
            prev_index = cursor;
            match memchr(b'\t', &self.line[cursor..]) {
                Some(i) => {
                    cursor += i + 1;
                }
                None => {
                    cursor = len;
                }
            }
            if cursor == 5 && &self.line[..cursor] != b"tskv\t" {
                return 0;
            }
            let mut value_offset = prev_index;
            if let Some(name_len) = memchr(b'=', &self.line[prev_index..cursor]) {
                value_offset += name_len;
            }
            if !self.initialized {
                self.i.push(Index {
                    value_offset: 0,
                    end: 0,
                });
                self.field_indexes.insert(
                    String::from_utf8_lossy(&self.line[prev_index..value_offset]).to_string(),
                    counter,
                );
            }
            self.i[counter].value_offset = value_offset + 1 /*skip '='*/;
            self.i[counter].end = cursor -1 /*exclude \t or \n */;
            counter += 1;
        }
        //if !self.initialized {
        //    println!("{:?}", self.field_indexes);
        //}
        self.initialized = true;
        counter
    }

    fn v(&self, key: &str) -> &[u8] {
        let idx = self.field_indexes.get(key).unwrap();
        &self.line[self.i[*idx].value_offset..self.i[*idx].end]
    }
}

fn main() {
    let mut counter = HashMap::new();
    let mut layout = Indexer {
        initialized: false,
        field_indexes: HashMap::new(),
        line: Vec::new(),
        i: Vec::new(),
    };

    let file = File::open("access.log").unwrap();
    let mut reader = BufReader::new(file);

    let mut line_len: usize;
    let mut count: u64;

    let re = Regex::new(r"^/[^/?:.-]{3,6}/").unwrap();
    let mut matched_requests = HashMap::new();

    loop {
        layout.line.truncate(0);
        match reader.read_until(b'\n', &mut layout.line) {
            Ok(0) => break,
            Ok(len) => {
                line_len = layout.parse_fields(len);
            }
            Err(err) => panic!("{}", err),
        }

        count = match counter.get(&line_len) {
            Some(c) => *c,
            None => 0,
        };
        counter.insert(line_len, count + 1);

        let req = &layout.v("request");
        let key = match re.captures(req) {
            Some(caps) => String::from_utf8_lossy(&caps[0]).to_string(),
            None => continue,
        };
        count = match matched_requests.get(&key) {
            Some(c) => *c,
            None => 0,
        };
        matched_requests.insert(key, count + 1);
    }
    println!("{:?}", counter);
    for (k, v) in &matched_requests {
        println!("{} {}", k, v);
    }
}
