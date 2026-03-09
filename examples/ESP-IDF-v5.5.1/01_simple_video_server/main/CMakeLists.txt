

set(srcs "main.c"
)
set(INCLUDE_DIRS 
    "."
)

set(html_files "../frontend/gzipped/index.html.gz"
               "../frontend/gzipped/loading.jpg.gz"
               "../frontend/gzipped/favicon.ico.gz"
               "../frontend/gzipped/assets/index.js.gz"
               "../frontend/gzipped/assets/index.css.gz")

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS ${INCLUDE_DIRS}
                       EMBED_TXTFILES ${html_files})
