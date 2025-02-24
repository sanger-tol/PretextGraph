#!/bin/bash

# init submodule
git submodule init
git submodule update

project_name="pretextgraph"
tag="0.0.7"

# archive main project
git archive --format=tar --prefix=${project_name}-${tag}/ -o ${project_name}-${tag}.tar HEAD

# archive submodules
for submodule in $(git submodule --quiet foreach 'echo $path'); do
    # enter and archive submodule
    (cd $submodule && git archive --format=tar --prefix=${project_name}-${tag}/$submodule/ HEAD) > "$submodule.tar"
    
    # add the tar to the main project tar
    tar --concatenate --file=${project_name}-${tag}.tar "$submodule.tar"
    
    rm "$submodule.tar"
done

gzip ${project_name}-${tag}.tar