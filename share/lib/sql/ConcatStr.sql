-- $Header$
CREATE OR REPLACE TYPE ConcatStrImpl AS OBJECT
(
	str		CLOB,

	STATIC FUNCTION ODCIAggregateInitialize(
			sctx		IN OUT ConcatStrImpl) 
		RETURN NUMBER,
	MEMBER FUNCTION ODCIAggregateIterate(
			self		IN OUT ConcatStrImpl,
			value		IN VARCHAR2)
		RETURN NUMBER,
	MEMBER FUNCTION ODCIAggregateTerminate(
			self		IN ConcatStrImpl,
			returnValue	OUT CLOB,
			flags		IN NUMBER)
		RETURN NUMBER,
	MEMBER FUNCTION ODCIAggregateMerge(
			self		IN OUT ConcatStrImpl,
			ctx2		IN ConcatStrImpl)
		RETURN NUMBER
);
/

CREATE OR REPLACE TYPE BODY ConcatStrImpl IS 
	STATIC FUNCTION ODCIAggregateInitialize(
		sctx			IN OUT ConcatStrImpl) 
	RETURN NUMBER IS 
	BEGIN
	  sctx := ConcatStrImpl(NULL);
	  sctx.str := '';
	  RETURN ODCIConst.Success;
	END;

	MEMBER FUNCTION ODCIAggregateIterate(
		self			IN OUT ConcatStrImpl,
		value			IN VARCHAR2) 
	RETURN NUMBER IS
	BEGIN
		IF LENGTH(self.str) > 0
		THEN
			self.str := self.str || ',' || value;
		ELSE
			self.str := self.str || value;
		END IF;
		RETURN ODCIConst.Success;
	END;

	MEMBER FUNCTION ODCIAggregateTerminate(
		self			IN ConcatStrImpl,
		returnValue		OUT CLOB,
		flags			IN NUMBER)
	RETURN NUMBER IS
	BEGIN
		returnValue := self.str;
		RETURN ODCIConst.Success;
	END;

	MEMBER FUNCTION ODCIAggregateMerge(
			self		IN OUT ConcatStrImpl,
			ctx2		IN ConcatStrImpl)
	RETURN NUMBER IS
	BEGIN
		self.str := self.str || ',' || ctx2.str;
	END;
END;
/

CREATE OR REPLACE FUNCTION ConcatStr(input VARCHAR2) RETURN CLOB
	PARALLEL_ENABLE AGGREGATE USING ConcatStrImpl;
/
