include "C.Grm"

% --> GRAMMAR REDEFINES <--

redefine ptr_operator
    * [repeat cv_qualifier] [SPON]
end define

% --> END REDEFINES <--

function main
	import TXLargs [repeat stringlit]
	replace $[program]
		PUT [program]
	export PUT
	export skip [number] 0
	export globalPassCounter [number] 0
	export localPassCounter [number] 0
	export callVal [number] 0
	deconstruct * TXLargs
		"-mut_out" Value [stringlit] MoreOptions [repeat stringlit]
	export mut_folder [stringlit] Value
	by
		PUT [driver]		
end function

function driver
	replace $[program]
		P [program]
	by
		P [mutateNULLAssignments 1]
		  [mutateFree 4]
		  [mutateCalloc 8]
		  [mutateAlloca 10]
		  [mutateRealloc 12]
		  [mutateMalloc 15]
end function

% --> MUTATION RULES <--

rule mutateNULLAssignments defValMax [number]
	import callVal [number]
	replace $[program]
		P [program]
	where callVal[<=defValMax]
	by
		P [removeNullDec 0]
		  [generateMutant "RMNA_" 0]
		  [prepareNextMutationOP 0]
		  
		  [removeNullStmt 1]
		  [generateMutant "RMNA_1" 1]
		  [prepareNextMutationOP 1]
end rule

rule mutateFree defValMax [number]
	import callVal [number]
	replace $[program]
		P [program]
	where callVal[<=defValMax]
	by
		P [removeFree 2]
		  [generateMutant "RMFS_2" 2]
		  [prepareNextMutationOP 2]
		  
		  [free_Increment_pointerAddr 3]
		  [generateMutant "FPIBO_2" 3]
		  [prepareNextMutationOP 3]
		  
		  [free_Decrement_pointerAddr 4]
		  [generateMutant "FPDBO_2" 4]
		  [prepareNextMutationOP 4]
		 
end rule

rule mutateCalloc defValMax [number]
	import callVal [number]
	replace $[program]
		P [program]
	where callVal[<=defValMax]
	by
		P [replace_calloc_W_malloc 5]
		  [generateMutant "REC2M_" 5]
		  [prepareNextMutationOP 5]
		  
		  [replace_calloc_W_alloca 6]
		  [generateMutant "REC2A_" 6]
		  [prepareNextMutationOP 6]
		  
		  [replace_calloc_W_NULL 7]
		  [generateMutant "REC2N_" 7]
		  [prepareNextMutationOP 7]
		  
		  [replace_calloc_W_NO 8]
		  [generateMutant "REC2NO_" 8]
		  [prepareNextMutationOP 8]
end rule

rule mutateAlloca defValMax [number]
	import callVal [number]
	replace $[program]
		P [program]
	where callVal[<=defValMax]
	by
		P [replace_alloca_W_NO 9]
		  [generateMutant "REA2NO_" 9]
		  [prepareNextMutationOP 9]
		  
		  [replace_alloca_W_NULL 10]
		  [generateMutant "REA2N_" 10]
		  [prepareNextMutationOP 10]
end rule

rule mutateRealloc defValMax [number]
	import callVal [number]
	replace $[program]
		P [program]
	where callVal[<=defValMax]
	by
		P [replace_realloc_W_NO 11]
		  [generateMutant "RER2NO_" 11]
		  [prepareNextMutationOP 11]
		  
		  [replace_realloc_W_NULL 12]
		  [generateMutant "RER2N_" 12]
		  [prepareNextMutationOP 12]
end rule

rule mutateMalloc defValMax [number]
	import callVal [number]
	replace $[program]
		P [program]
	where callVal[<=defValMax]
	by
		P 
		  [replace_malloc_W_alloca 13]
		  [generateMutant "REM2A_2" 13]
		  [prepareNextMutationOP 13]
		  
		  [replace_malloc_W_NO 14]
		  [generateMutant "REM2NO_" 14]
		  [prepareNextMutationOP 14]
		  
		  [replace_malloc_W_N 15]
		  [generateMutant "REM2N_" 15]
		  [prepareNextMutationOP 15]
		  
end rule

rule replace_malloc_W_NO defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $[assignment_expression]
		'malloc '( AE[list argument_expression] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	by
		'-1
end rule

rule replace_malloc_W_N defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $[assignment_expression]
		'malloc '( AE[list argument_expression] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	by
		'NULL
end rule

rule replace_malloc_W_alloca defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $[assignment_expression]
		'malloc '( AL [list argument_expression] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	by
		'alloca '( AL ')
end rule

rule replace_realloc_W_NO defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $ [assignment_expression]
		'realloc '( AE1 [argument_expression],AE2 [argument_expression] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	by
		'-1
end rule

rule replace_realloc_W_NULL defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $ [assignment_expression]
		'realloc '( AE1 [argument_expression],AE2 [argument_expression] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	by
		'NULL
end rule

rule replace_alloca_W_NULL defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $ [assignment_expression]
		'alloca '( AL[list argument_expression] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	by
		'NULL
end rule

rule replace_alloca_W_NO defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $ [assignment_expression]
		'alloca '( AL[list argument_expression] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	by
		'-1
end rule

rule replace_calloc_W_NO defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $ [assignment_expression]
		'calloc '( AE1 [argument_expression],AE2 [argument_expression] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	by
		'-1
end rule

rule replace_calloc_W_NULL defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $ [assignment_expression]
		'calloc '( AE1 [argument_expression],AE2 [argument_expression] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	by
		'NULL
end rule

rule replace_calloc_W_malloc defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $ [assignment_expression]
		'calloc '( AE1 [assignment_expression],AE2 [assignment_expression] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	construct E1 [expression] AE1
	construct E2 [expression] AE2
	by
		'malloc '('(E1') * '(E2') ')
end rule 

rule replace_calloc_W_alloca defValMax [number] 
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $ [assignment_expression]
		'calloc '( AE1 [assignment_expression],AE2 [assignment_expression] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	construct E1 [expression] AE1
	construct E2 [expression] AE2
	by
		'alloca '('(E1') * '(E2') ')
end rule 

rule removeNullDec defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $ [init_declarator]
		D [declarator]'= 'NULL	
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	by
		D 
end rule

rule removeNullStmt defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]

	replace $ [statement]
		AE [assignment_expression] '= 'NULL ';
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	by
		AE ';
end rule

rule removeFree defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace [repeat declaration_or_statement]
		Head[declaration_or_statement] 
		Tail[repeat declaration_or_statement]
	deconstruct Head 'free '( MA [macro_arguments] ') ';
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	by
		Tail
end rule

rule free_Increment_pointerAddr defValMax [number]
	import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $[macro_call]
		'free '( MA [macro_arguments] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	by
		'free '( '(MA')+1') 
end rule

rule free_Decrement_pointerAddr defValMax [number]
		import globalPassCounter [number]
	import localPassCounter [number]
	import skip [number]
	import callVal [number]
	
	replace $[macro_call]
		'free '( MA [macro_arguments] ')
	where callVal[<=defValMax]
	export localPassCounter localPassCounter[+1] 
	where skip[=0]
	where localPassCounter[>globalPassCounter]
	export skip 1
	by
		'free '( '(MA')-1') 
end rule

% --> SAVE AND REVERT PUT to original after every mutation <--

rule generateMutant MC [stringlit] defValMax [number]
	import globalPassCounter [number] 
	import localPassCounter [number]
	import skip [number] 
	import callVal [number]
	import mut_folder [stringlit]
    
	replace $ [program]
		P [program]	 
	where callVal[<=defValMax]
	where localPassCounter[>globalPassCounter]
    construct out [stringlit]  mut_folder [+"/"]
    construct MI [stringlit] MC [quote globalPassCounter][+".c"]
    construct final_output [stringlit] out [+ MI]
	export globalPassCounter globalPassCounter[+1]
	export localPassCounter 0
	export skip 0
	by
		P [write final_output][revertTree] [driver]
end rule

rule revertTree
	import PUT [program]
	replace $ [program]
		P [program]
	by
		PUT
end rule

rule prepareNextMutationOP defValMax [number]
	import globalPassCounter [number] 
	import localPassCounter [number]
	import skip [number] 
	import callVal [number]
	
	replace $ [program]
		P [program]
	where callVal[<=defValMax]
	export globalPassCounter 0
	export localPassCounter 0
	export skip 0
	export callVal callVal[+1]
	by
		P
end rule

