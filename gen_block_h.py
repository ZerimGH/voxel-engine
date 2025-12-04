#!/usr/bin/env python

# Python script to read the file "blocks.txt", and generate src/blocks.h
# if all the block textures exist

def file_exists(path):
    try: file = open(path)
    except: return False
    file.close()
    return True

def gen_texture_list(blocks):
    out = ""
    num_blocks = len(blocks)
    count = 0
    for block_name in blocks:
        top = "textures/" + block_name + "-top.png" 
        side = "textures/" + block_name + "-side.png" 
        bottom = "textures/" + block_name + "-bottom.png" 
        if not file_exists(top):
            return None
        else:
            out += "\"" + top + "\", " 

        if not file_exists(side):
            return None
        else:
            out += "\"" + side + "\", " 

        if not file_exists(bottom):
            return None
        else:
            out += "\"" + bottom + ("\", " if count < num_blocks - 1 else "\"")
        out += '\\\n' if count < num_blocks - 1 else "\n"

        count += 1
    return out

def gen_block_h(blocks):
    out = ""
    out += "#ifndef BLOCK_h"
    out += "\n\n"
    out += "#define BLOCK_h"
    out += "\n\n"
    out += "#define NUM_BLOCKS " + str(len(blocks))
    out += "\n\n"
    out += "#define NUM_BLOCK_TEXTURES (NUM_BLOCKS * 3)"
    out += "\n\n"
    out += "#define BLOCK_TEXTURES \\"
    out += "\n"
    texture_list = gen_texture_list(blocks)
    if texture_list is None: return None
    out += texture_list

    out +=  """
#include <stdint.h>

typedef uint8_t BlockType;

enum {
  BlockAir,
"""
    for block in blocks:
        out += "  Block" + block + ",\n";
    out +="""
};

typedef struct {
  BlockType type;
} Block;
"""

    out += "\n#endif"
    return out

def main():
    try: file = open("blocks.txt")
    except: return 1;
    file_str = file.read() 
    file.close()
    if file_str is None: return 1
    blocks = file_str.splitlines()
    block_h = gen_block_h(blocks)
    if block_h is None: return 1
    with open("src/block.h", "w") as text_file:
        text_file.write(block_h)
    return 0

if __name__ == "__main__":
    status = main()
    if status != 0:
        print("Failed to generate src/blocks.h, you're probably missing texture files.");
    exit(status)
