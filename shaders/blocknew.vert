layout(location = 0) in uint a; 
layout(location = 0) in uint b; 

// ...

void main() {
  uint read = a;
  vec3 off = vec3(0, 0, 0);
  for(int i = 0; i < 3; i++) {
    off[i] = float(read & 0x000000FF);
    read <<= 8;
  }
  // 24 bits removed, 8 remaining 
  vec2 tex = vec2(0, 0);
  tex[0] = float(read & 0x00000001);
  tex <<= 1;
  tex[1] = float(read & 0x00000001);
  tex <<= 1;
  // 26 bits removed, 6 remaining
  uint side_index = 0;
  side_index = read & 0x00000007;
  read <<= 3;
  // 29 bits removed, 3 remaining
  read |= b << 3;
  // 32 remaining
  uint block_type = 0;
  block_type = read & 0x000000FF;
  read <<= 8;
  // 24 remaining
  // done :)
}
