/*
  Copyright (c) 2004-2022 The FlameRobin Development Team

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/


#ifndef FR_FUNCTION_H
#define FR_FUNCTION_H

#include "metadata/collection.h"
#include "metadata/privilege.h"


class ProgressIndicator;

class Function: public MetadataItem
{
private:
	std::vector<Privilege> privilegesM;
	ParameterPtrs parametersM;
protected:

	virtual void loadChildren();
	virtual void lockChildren();
	virtual void unlockChildren();

public:
    Function(DatabasePtr database, const wxString& name);
    Function(MetadataItem* parent, const wxString& name);

	bool getChildren(std::vector<MetadataItem *>& temp);


	ParameterPtrs::iterator begin();
	ParameterPtrs::iterator end();
	ParameterPtrs::const_iterator begin() const;
	ParameterPtrs::const_iterator end() const;

	size_t getParamCount() const;
	ParameterPtr findParameter(const wxString& name) const;

	virtual wxString getDefinition();
	wxString getOwner();
	virtual wxString getSource() = 0;
	wxString getSqlSecurity();
	virtual const wxString getTypeName()  const = 0;
	std::vector<Privilege>* getPrivileges();

    virtual void acceptVisitor(MetadataItemVisitor* visitor);
    virtual void checkDependentFunction();
};

class UDF : public Function
{
private:
	wxString libraryNameM, entryPointM, definitionM, retstrM, paramListM;
protected:
	virtual void loadProperties();
public:
	UDF(DatabasePtr database, const wxString& name);

	virtual wxString getCreateSql();
	virtual wxString getDropSqlStatement() const;
	virtual wxString getDefinition();
	virtual wxString getSource();
	virtual const wxString getTypeName() const ;

	wxString getEntryPoint();
	wxString getLibraryName();

	virtual void acceptVisitor(MetadataItemVisitor* visitor);
};

class FunctionSQL : public Function
{
private:

protected:
    bool deterministicM;
	virtual void loadProperties();
public:
	FunctionSQL(DatabasePtr database, const wxString& name);
    FunctionSQL(MetadataItem* parent, const wxString& name);

	wxString getSource();
	wxString getAlterSql(bool full = true);
	virtual wxString getDefinition();   // used for calltip in sql editor
	virtual const wxString getTypeName() const;
	virtual void acceptVisitor(MetadataItemVisitor* visitor);
    virtual wxString getQuotedName() const;

};

class UDFs : public MetadataCollection<UDF>
{
protected:
	virtual void loadChildren();
public:
	UDFs(DatabasePtr database);

	virtual void acceptVisitor(MetadataItemVisitor* visitor);
	void load(ProgressIndicator* progressIndicator);
	virtual const wxString getTypeName() const;

};

class FunctionSQLs: public MetadataCollection<FunctionSQL>
{
protected:
    virtual void loadChildren();
public:
    FunctionSQLs(DatabasePtr database);

    virtual void acceptVisitor(MetadataItemVisitor* visitor);
    void load(ProgressIndicator* progressIndicator);
    virtual const wxString getTypeName() const;
};

#endif
