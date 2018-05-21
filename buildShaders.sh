#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

shaders=( "morph.vert" "morph.frag", "normal.vert" )

for i in "${shaders}"
do
    glslc ${DIR}/data/shaders/${i} -o ${DIR}/data/shaders/${i}.spv;
done

