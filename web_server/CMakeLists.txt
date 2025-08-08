USTC_CG_ADD_LIB(
    web_server
    SHARED
    PUBLIC_LIBS nodes_core nodes_system
    PRIVATE_LIBS httplib::httplib
)