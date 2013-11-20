aps/C.c:	mcp = node->mcprec->value;
aps/C.c:	module = ValueStringPointer(GetItemLongName(node->mcprec->value,"dc.module"));
aps/C.c:		if		(  ValueInteger(GetItemLongName(node->mcprec->value,"rc"))  <  0  ) {
aps/Exec.c:	printf("mcp = [%s]\n",node->mcprec->name);
aps/Exec.c:	//	DumpValueStruct(node->mcprec->value); 
aps/Exec.c:	ConvSetRecName(handler->conv,node->mcprec->name);
aps/Exec.c:	size = conv->SizeValue(handler->conv,node->mcprec->value);
aps/Exec.c:	conv->PackValue(handler->conv,LBS_Body(iobuff),node->mcprec->value);
aps/Exec.c:	if		(  node->linkrec  !=  NULL  ) {
aps/Exec.c:		ConvSetRecName(handler->conv,node->linkrec->name);
aps/Exec.c:		size = conv->SizeValue(handler->conv,node->linkrec->value);
aps/Exec.c:		conv->PackValue(handler->conv,LBS_Body(iobuff),node->linkrec->value);
aps/Exec.c:	if		(  node->sparec  !=  NULL  ) {
aps/Exec.c:		ConvSetRecName(handler->conv,node->sparec->name);
aps/Exec.c:		size = conv->SizeValue(handler->conv,node->sparec->value);
aps/Exec.c:		conv->PackValue(handler->conv,LBS_Body(iobuff),node->sparec->value);
aps/Exec.c:	for	( i = 0 ; i < node->cWindow ; i ++ ) {
aps/Exec.c:		if		(  node->scrrec[i]  !=  NULL  ) {
aps/Exec.c:			ConvSetRecName(handler->conv,node->scrrec[i]->name);
aps/Exec.c:			size = conv->SizeValue(handler->conv,node->scrrec[i]->value);
aps/Exec.c:			conv->PackValue(handler->conv,LBS_Body(iobuff),node->scrrec[i]->value);
aps/Exec.c:	ConvSetRecName(handler->conv,node->mcprec->name);
aps/Exec.c:	conv->UnPackValue(handler->conv,LBS_Body(iobuff),node->mcprec->value);
aps/Exec.c:	if		(  node->linkrec  !=  NULL  ) {
aps/Exec.c:		ConvSetRecName(handler->conv,node->linkrec->name);
aps/Exec.c:		conv->UnPackValue(handler->conv,LBS_Body(iobuff),node->linkrec->value);
aps/Exec.c:	if		(  node->sparec  !=  NULL  ) {
aps/Exec.c:		ConvSetRecName(handler->conv,node->sparec->name);
aps/Exec.c:		conv->UnPackValue(handler->conv,LBS_Body(iobuff),node->sparec->value);
aps/Exec.c:	for	( i = 0 ; i < node->cWindow ; i ++ ) {
aps/Exec.c:		if		(	(  node->scrrec[i]         !=  NULL  )
aps/Exec.c:				&&	(  node->scrrec[i]->value  !=  NULL  ) ) {
aps/Exec.c:			ConvSetRecName(handler->conv,node->scrrec[i]->name);
aps/Exec.c:			conv->UnPackValue(handler->conv,LBS_Body(iobuff),node->scrrec[i]->value);
aps/Exec.c:	module = ValueToString(GetItemLongName(node->mcprec->value,"dc.module"),NULL);
aps/OpenCOBOL.c:	if		(  node->mcprec  !=  NULL  ) {
aps/OpenCOBOL.c:		OpenCOBOL_PackValue(OpenCOBOL_Conv,McpData,node->mcprec->value);
aps/OpenCOBOL.c:	if		(  node->linkrec  !=  NULL  ) {
aps/OpenCOBOL.c:		OpenCOBOL_PackValue(OpenCOBOL_Conv,LinkData,node->linkrec->value);
aps/OpenCOBOL.c:	if		(  node->sparec  !=  NULL  ) {
aps/OpenCOBOL.c:		OpenCOBOL_PackValue(OpenCOBOL_Conv,SpaData,node->sparec->value);
aps/OpenCOBOL.c:	for	( i = 0 , scr = (char *)ScrData ; i < node->cWindow ; i ++ ) {
aps/OpenCOBOL.c:		if		(  node->scrrec[i]  !=  NULL  ) {
aps/OpenCOBOL.c:			size = OpenCOBOL_PackValue(OpenCOBOL_Conv,scr,node->scrrec[i]->value);
aps/OpenCOBOL.c:			if ((sch = g_hash_table_lookup(CacheScreen,node->scrrec[i]->name)) != NULL){
aps/OpenCOBOL.c:	if		(  node->mcprec  !=  NULL  ) {
aps/OpenCOBOL.c:		OpenCOBOL_UnPackValue(OpenCOBOL_Conv,McpData,node->mcprec->value);
aps/OpenCOBOL.c:	if		(  node->linkrec  !=  NULL  ) {
aps/OpenCOBOL.c:		OpenCOBOL_UnPackValue(OpenCOBOL_Conv,LinkData,node->linkrec->value);
aps/OpenCOBOL.c:	if		(  node->sparec  !=  NULL  ) {
aps/OpenCOBOL.c:		OpenCOBOL_UnPackValue(OpenCOBOL_Conv,SpaData,node->sparec->value);
aps/OpenCOBOL.c:	for	( i = 0 , scr = (char *)ScrData ; i < node->cWindow ; i ++ ) {
aps/OpenCOBOL.c:		if		(  node->scrrec[i]  !=  NULL  ) {
aps/OpenCOBOL.c:			if ((size = IsCacheScreen(node->scrrec[i]->name, scr)) != 0) {
aps/OpenCOBOL.c:				scr += OpenCOBOL_UnPackValue(OpenCOBOL_Conv,scr,node->scrrec[i]->value);
aps/OpenCOBOL.c:	module = ValueStringPointer(GetItemLongName(node->mcprec->value,"dc.module"));
aps/OpenCOBOL.c:		TimerPrintf(start,end, "OpenCOBOL %s:%s:%s\n",module, node->widget, node->event);
aps/OpenCOBOL.c:		if		(  ValueInteger(GetItemLongName(node->mcprec->value,"rc"))  <  0  ) {
aps/Resident.c://	DumpValueStruct(node->mcprec->value); 
aps/Resident.c:	ConvSetRecName(handler->conv,node->mcprec->name);
aps/Resident.c:	size = conv->SizeValue(handler->conv,node->mcprec->value);
aps/Resident.c:	conv->PackValue(handler->conv,LBS_Body(iobuff),node->mcprec->value);
aps/Resident.c:	if		(  node->linkrec  !=  NULL  ) {
aps/Resident.c:		ConvSetRecName(handler->conv,node->linkrec->name);
aps/Resident.c:		size = conv->SizeValue(handler->conv,node->linkrec->value);
aps/Resident.c:		conv->PackValue(handler->conv,LBS_Body(iobuff),node->linkrec->value);
aps/Resident.c:	if		(  node->sparec  !=  NULL  ) {
aps/Resident.c:		ConvSetRecName(handler->conv,node->sparec->name);
aps/Resident.c:		size = conv->SizeValue(handler->conv,node->sparec->value);
aps/Resident.c:		conv->PackValue(handler->conv,LBS_Body(iobuff),node->sparec->value);
aps/Resident.c:	for	( i = 0 ; i < node->cWindow ; i ++ ) {
aps/Resident.c:		if		(  node->scrrec[i]  !=  NULL  ) {
aps/Resident.c:			ConvSetRecName(handler->conv,node->scrrec[i]->name);
aps/Resident.c:			size = conv->SizeValue(handler->conv,node->scrrec[i]->value);
aps/Resident.c:			conv->PackValue(handler->conv,LBS_Body(iobuff),node->scrrec[i]->value);
aps/Resident.c:	ConvSetRecName(handler->conv,node->mcprec->name);
aps/Resident.c:	conv->UnPackValue(handler->conv,LBS_Body(iobuff),node->mcprec->value);
aps/Resident.c:	if		(  node->linkrec  !=  NULL  ) {
aps/Resident.c:		ConvSetRecName(handler->conv,node->linkrec->name);
aps/Resident.c:		conv->UnPackValue(handler->conv,LBS_Body(iobuff),node->linkrec->value);
aps/Resident.c:	if		(  node->sparec  !=  NULL  ) {
aps/Resident.c:		ConvSetRecName(handler->conv,node->sparec->name);
aps/Resident.c:		conv->UnPackValue(handler->conv,LBS_Body(iobuff),node->sparec->value);
aps/Resident.c:	for	( i = 0 ; i < node->cWindow ; i ++ ) {
aps/Resident.c:		if		(  node->scrrec[i]  !=  NULL  ) {
aps/Resident.c:			ConvSetRecName(handler->conv,node->scrrec[i]->name);
aps/Resident.c:			conv->UnPackValue(handler->conv,LBS_Body(iobuff),node->scrrec[i]->value);
aps/Resident.c:	module = ValueStringPointer(GetItemLongName(node->mcprec->value,"dc.module"));
aps/Ruby.c:	if		(  node->mcprec  !=  NULL  ) {
aps/Ruby.c:		buf = xmalloc(JSON_SizeValue(RubyConv,node->mcprec->value));
aps/Ruby.c:		JSON_PackValue(RubyConv,buf,node->mcprec->value);
aps/Ruby.c:	if		(  node->linkrec  !=  NULL  ) {
aps/Ruby.c:		buf = xmalloc(JSON_SizeValue(RubyConv,node->linkrec->value));
aps/Ruby.c:		JSON_PackValue(RubyConv,buf,node->linkrec->value);
aps/Ruby.c:	if		(  node->sparec  !=  NULL  ) {
aps/Ruby.c:		buf = xmalloc(JSON_SizeValue(RubyConv,node->sparec->value));
aps/Ruby.c:		JSON_PackValue(RubyConv,buf,node->sparec->value);
aps/Ruby.c:	for	( i = 0; i < node->cWindow ; i ++ ) {
aps/Ruby.c:		if		(  node->scrrec[i]  !=  NULL  ) {
aps/Ruby.c:			buf = xmalloc(JSON_SizeValue(RubyConv,node->scrrec[i]->value));
aps/Ruby.c:			JSON_PackValue(RubyConv,buf,node->scrrec[i]->value);
aps/Ruby.c:			rb_hash_aset(node_info,rb_str_new2(node->scrrec[i]->name),rb_str_new2(buf));
aps/Ruby.c:	if		(  node->mcprec  !=  NULL  ) {
aps/Ruby.c:			JSON_UnPackValue(RubyConv,StringValueCStr(obj),node->mcprec->value);
aps/Ruby.c:	if		(  node->linkrec  !=  NULL  ) {
aps/Ruby.c:			JSON_UnPackValue(RubyConv,StringValueCStr(obj),node->linkrec->value);
aps/Ruby.c:	if		(  node->sparec  !=  NULL  ) {
aps/Ruby.c:			JSON_UnPackValue(RubyConv,StringValueCStr(obj),node->sparec->value);
aps/Ruby.c:	for	( i = 0; i < node->cWindow ; i ++ ) {
aps/Ruby.c:		if		(  node->scrrec[i]  !=  NULL  ) {
aps/Ruby.c:			obj = rb_hash_aref(node_info,rb_str_new2(node->scrrec[i]->name));
aps/Ruby.c:				JSON_UnPackValue(RubyConv,StringValueCStr(obj),node->scrrec[i]->value);
aps/Ruby.c:	dc_module_value = GetItemLongName(node->mcprec->value, "dc.module");
aps/Ruby.c:	if (ValueInteger(GetItemLongName(node->mcprec->value, "rc")) < 0) {
aps/aps_main.c:	node->mcprec = ThisEnv->mcprec;
aps/aps_main.c:	node->linkrec = ThisEnv->linkrec;
aps/aps_main.c:	node->sparec = ThisLD->sparec;
aps/aps_main.c:	node->cWindow = ThisLD->cWindow;
aps/aps_main.c:	node->cBind = ThisLD->cBind;
aps/aps_main.c:	node->bhash = ThisLD->bhash;
aps/aps_main.c:	node->textsize = ThisLD->textsize;
aps/aps_main.c:	node->scrrec = (RecordStruct **)xmalloc(sizeof(RecordStruct *) * node->cWindow);
aps/aps_main.c:	node->dbstatus = GetDBRedirectStatus(0);
aps/aps_main.c:	for	( i = 0 ; i < node->cWindow ; i ++ ) {
aps/aps_main.c:		node->scrrec[i] = ThisLD->windows[i];
aps/aps_main.c:	xfree(node->scrrec);
aps/aps_main.c:		dbgprintf("window = [%s]",ValueStringPointer(GetItemLongName(node->mcprec->value,"dc.window")));
aps/aps_main.c:					GetItemLongName(node->mcprec->value,"dc.window"));
aps/aps_main.c:		SetValueString(GetItemLongName(node->mcprec->value,"dc.module"),bind->module,NULL);
aps/aps_main.c:		if (node->dbstatus == DB_STATUS_REDFAILURE) {
aps/aps_main.c:			node->dbstatus = GetDBRedirectStatus(0);
aps/aps_main.c:		strcpy(CurrentUser, ValueStringPointer(GetItemLongName(node->mcprec->value,"dc.user")));
aps/aps_main.c:		strcpy(CurrentTerm, ValueStringPointer(GetItemLongName(node->mcprec->value,"dc.term")));
aps/apsio.c:			e = node->mcprec->value;
aps/apsio.c:			node->command = hdr.command;
aps/apsio.c:			node->dbstatus = hdr.dbstatus;
aps/apsio.c:			strcpy(node->uuid,hdr.uuid);
aps/apsio.c:			strcpy(node->user,hdr.user);
aps/apsio.c:			strcpy(node->window,hdr.window);
aps/apsio.c:			strcpy(node->widget,hdr.widget);
aps/apsio.c:			strcpy(node->event,hdr.event);
aps/apsio.c:			node->w.sp = RecvInt(fp);			ON_IO_ERROR(fp,badio);
aps/apsio.c:			for (i=0;i<node->w.sp ;i++) {
aps/apsio.c:				node->w.s[i].puttype = RecvChar(fp);	ON_IO_ERROR(fp,badio);
aps/apsio.c:				RecvnString(fp,SIZE_NAME,node->w.s[i].window);
aps/apsio.c:			RecvRecord(fp,node->linkrec);		ON_IO_ERROR(fp,badio);
aps/apsio.c:			RecvRecord(fp,node->sparec);		ON_IO_ERROR(fp,badio);
aps/apsio.c:			for	( i = 0 ; i < node->cWindow ; i ++ ) {
aps/apsio.c:				RecvRecord(fp,node->scrrec[i]);		ON_IO_ERROR(fp,badio);
aps/apsio.c:	bind = g_hash_table_lookup(node->bhash, hdr.window);
aps/apsio.c:		e = node->mcprec->value;
aps/apsio.c:		strcpy(node->uuid,hdr.uuid);
aps/apsio.c:		strcpy(node->user,hdr.user);
aps/apsio.c:		strcpy(node->window,hdr.window);
aps/apsio.c:		for(i = 0; i < node->cWindow; i++) {
aps/apsio.c:			if (node->scrrec[i] != NULL &&
aps/apsio.c:				!strcmp(node->scrrec[i]->name, hdr.window)) {
aps/apsio.c:				e = node->scrrec[i]->value;
aps/apsio.c:		node->messageType = MESSAGE_TYPE_TERM;
aps/apsio.c:		node->messageType = MESSAGE_TYPE_API;
aps/apsio.c:	e = node->mcprec->value;
aps/apsio.c:	SendChar(fp,node->dbstatus);					ON_IO_ERROR(fp,badio);
aps/apsio.c:	SendChar(fp,node->puttype);						ON_IO_ERROR(fp,badio);
aps/apsio.c:			SendInt(fp,node->w.sp);					ON_IO_ERROR(fp,badio);
aps/apsio.c:			for	(i=0;i<node->w.sp;i++) {
aps/apsio.c:				SendChar(fp,node->w.s[i].puttype);
aps/apsio.c:				SendString(fp,node->w.s[i].window);
aps/apsio.c:			SendRecord(fp, node->linkrec);			ON_IO_ERROR(fp,badio);
aps/apsio.c:			SendRecord(fp, node->sparec);			ON_IO_ERROR(fp,badio);
aps/apsio.c:				SendRecord(fp,node->scrrec[i]);		ON_IO_ERROR(fp,badio);
aps/apsio.c:	for(i = 0; i < node->cWindow; i++) {
aps/apsio.c:		if (node->scrrec[i] != NULL &&
aps/apsio.c:			!strcmp(node->scrrec[i]->name, node->window)) {
aps/apsio.c:			SendRecord(fp, node->scrrec[i]); ON_IO_ERROR(fp,badio);
aps/apsio.c:	if (node->messageType == MESSAGE_TYPE_API) {
aps/apslib.c:	mcp = node->mcprec->value;
aps/apslib.c:	mcp = node->mcprec->value;
aps/apslib.c:	bind = (WindowBind *)g_hash_table_lookup(node->bhash,name);
aps/apslib.c:	status = ValueStringPointer(GetItemLongName(node->mcprec->value,"dc.status"));
aps/apslib.c:	event = ValueStringPointer(GetItemLongName(node->mcprec->value,"dc.event"));
aps/handler.c:	for (i=0;i<node->w.sp;i++) {
aps/handler.c:			i,node->w.s[i].window,strputtype(node->w.s[i].puttype));
aps/handler.c:	mcp = node->mcprec->value; 
aps/handler.c:		CommandString(node->command),NULL);
aps/handler.c:	dbgprintf("node->dbstatus = %d",node->dbstatus);
aps/handler.c:		DBSTATUS[(int)node->dbstatus],NULL);
aps/handler.c:	node->thisscrrec = bind->rec;
aps/handler.c:	for (i=0;i<node->w.sp;i++) {
aps/handler.c:		if (node->w.s[i].puttype != SCREEN_NULL) {
aps/handler.c:			w.s[w.sp].puttype = node->w.s[i].puttype;
aps/handler.c:			strncpy(w.s[w.sp].window,node->w.s[i].window,SIZE_NAME);
aps/handler.c:	node->w.sp = w.sp;
aps/handler.c:		node->w.s[i].puttype = w.s[i].puttype;
aps/handler.c:		strncpy(node->w.s[i].window,w.s[i].window,SIZE_NAME);
aps/handler.c:	for (i=0;i<node->w.sp;i++) {
aps/handler.c:		if (!strcmp(node->window,node->w.s[i].window)) {
aps/handler.c:	if (sp == -1 && strlen(node->window) > 0) {
aps/handler.c:		if (node->w.sp > WINDOW_STACK_SIZE) {
aps/handler.c:				WINDOW_STACK_SIZE,node->window);
aps/handler.c:			for(i=0;i<node->w.sp;i++) {
aps/handler.c:					i,node->w.s[i].window,node->w.s[i].puttype);
aps/handler.c:		strncpy(node->w.s[node->w.sp].window,node->window,SIZE_NAME);
aps/handler.c:		node->w.s[node->w.sp].puttype = node->puttype;
aps/handler.c:		node->w.sp++;
aps/handler.c:	for (i=0;i<node->w.sp;i++) {
aps/handler.c:			i,node->w.s[i].window,strputtype(node->w.s[i].puttype));
aps/handler.c:	for (i=0;i<node->w.sp;i++) {
aps/handler.c:			i,node->w.s[i].window,strputtype(node->w.s[i].puttype));
aps/handler.c:		GetItemLongName(node->mcprec->value,"dc.window"));
aps/handler.c:		GetItemLongName(node->mcprec->value,"dc.puttype"));
aps/handler.c:		node->puttype = SCREEN_CURRENT_WINDOW;
aps/handler.c:		node->puttype = SCREEN_NEW_WINDOW;
aps/handler.c:		node->puttype = SCREEN_CLOSE_WINDOW;
aps/handler.c:		node->puttype = SCREEN_CHANGE_WINDOW;
aps/handler.c:		node->puttype = SCREEN_JOIN_WINDOW;
aps/handler.c:		node->puttype = SCREEN_CURRENT_WINDOW;
aps/handler.c:		node->window,dc_window,strputtype(node->puttype));
aps/handler.c:	switch (node->puttype) {
aps/handler.c:		for (i=0;i<node->w.sp;i++) {
aps/handler.c:			if (!strcmp(node->window,node->w.s[i].window)) {
aps/handler.c:				node->w.s[i].puttype = SCREEN_CLOSE_WINDOW;
aps/handler.c:		if (node->w.sp >= 1) {
aps/handler.c:			node->w.s[node->w.sp-1].puttype = SCREEN_CLOSE_WINDOW;
aps/handler.c:		for (i=0;i<node->w.sp;i++) {
aps/handler.c:			if (!strcmp(dc_window,node->w.s[i].window)) {
aps/handler.c:			for (i=0;i<node->w.sp;i++) {
aps/handler.c:				node->w.s[i].puttype = SCREEN_CLOSE_WINDOW;
aps/handler.c:			for (i=sp+1;i<node->w.sp;i++) {
aps/handler.c:				node->w.s[i].puttype = SCREEN_CLOSE_WINDOW;
aps/handler.c:	for (i=0;i<node->w.sp;i++) {
aps/handler.c:			i,node->w.s[i].window,strputtype(node->w.s[i].puttype));
aps/handler.c:		GetItemLongName(node->mcprec->value,"dc.window"));
aps/handler.c:		bind->module, node->widget, node->event);
aps/handler.c:			bind->module, node->widget, node->event);
aps/handler.c:			bind->module, node->widget, node->event);
aps/handler.c:		node->user,bind->module,window,node->widget, node->event);
dblib/XMLIO.c:			node = node->next;
dblib/XMLIO.c:			for( node = root->children; node != NULL; node = node->next) {
dblib/XMLIO.c:	for (node=root->children, i=0; node != NULL; node=node->next, i++) {
dblib/XMLIO2.c:		for(node=root->children;node!=NULL;node=node->next) {
dblib/XMLIO2.c:			if (node->type != XML_ELEMENT_NODE) {
dblib/XMLIO2.c:			for( node = root->children; node != NULL; node = node->next) {
dblib/XMLIO2.c:				if (!xmlStrcmp(node->name, ValueRecordName(val, i))) {
dblib/XMLIO2.c:	for (node=root->children, i=0; node != NULL; node=node->next, i++) {
glclient/bd_config.c:  for (node = root->children; node != NULL; node = node->next)
glclient/bd_config.c:      if (xmlStrcmp (node->name, "entry") != 0)
glclient/bd_config.c:      contents = xmlNodeListGetString(node->doc, node->children, TRUE);
glclient/bd_config.c:  for (node = root->children; node != NULL; node = node->next)
glclient/bd_config.c:      if (xmlStrcmp (node->name, "section") == 0)
