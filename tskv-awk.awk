#!/usr/bin/mawk -f

function parse_fields(){
  if ($1 != "tskv") { return 0 } # 0 == false - skip invalid lines
  if (!initialized) {
    for (i=2;i<=NF;i++) {
      eq_idx=index($i, "=")
      value_indexes[i]=eq_idx+1
      field_indexes[substr($i, 1, eq_idx-1)]=i
    }
    initialized=1
  }
  return initialized
}

function v(name){
  num=field_indexes[name]
  return substr($num, value_indexes[num])
}

BEGIN {
  FS="\t"
  request_re="^/[^/?:.-][^/?:.-][^/?:.-][^/?:.-]?[^/?:.-]?[^/?:.-]?/"  # PCRE "^/[^/?:.-]{3,6}/
}
{
  while (!parse_fields()) { next }

  if (match(v("request"), request_re) > 0) {
    matched_requests[substr(v("request"), 1, RLENGTH)]++
  }
  x[NF]++
}
END {
  for (i in x)
    print i, x[i]
  for (i in matched_requests)
    printf "%s %s\n", i, matched_requests[i]
}
