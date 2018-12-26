module api.c {
	method api_create_message {
	   input {
	      const char *sender;
	      const char *dest;
	      const char *cmd;
	   }
	   returns APImsg *;
	};

	method api_destroy_message {
	   input {
	      APImsg *msg;
	   }
	   returns int;
	};

	method api_garbagecollect {
	   returns null;
	};

	
}
