//
// TableRenderer.cpp
//
// $Id: //poco/Main/WebWidgets/ExtJS/src/TableRenderer.cpp#4 $
//
// Library: ExtJS
// Package: Core
// Module:  TableRenderer
//
// Copyright (c) 2007, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//


#include "Poco/WebWidgets/ExtJS/TableRenderer.h"
#include "Poco/WebWidgets/ExtJS/FormRenderer.h"
#include "Poco/WebWidgets/ExtJS/Utility.h"
#include "Poco/WebWidgets/ExtJS/TableCellHandlerFactory.h"
#include "Poco/WebWidgets/Table.h"
#include "Poco/WebWidgets/WebApplication.h"
#include "Poco/WebWidgets/RequestHandler.h"
#include "Poco/WebWidgets/DateFormatter.h"
#include <sstream>


namespace Poco {
namespace WebWidgets {
namespace ExtJS {


TableRenderer::TableRenderer()
{
}


TableRenderer::~TableRenderer()
{
}


void TableRenderer::renderHead(const Renderable* pRenderable, const RenderContext& context, std::ostream& ostr)
{
	poco_assert_dbg (pRenderable != 0);
	poco_assert_dbg (pRenderable->type() == typeid(Poco::WebWidgets::Table));
	const Table* pTable = static_cast<const Poco::WebWidgets::Table*>(pRenderable);

	ostr << "new Ext.grid.EditorGridPanel({";

	TableRenderer::renderProperties(pTable, context, ostr);

	ostr << "})";
}


void TableRenderer::renderBody(const Renderable* pRenderable, const RenderContext& context, std::ostream& ostr)
{
}


void TableRenderer::renderProperties(const Table* pTable, const RenderContext& context, std::ostream& ostr)
{
	WebApplication& app = WebApplication::instance();
	Renderable::ID id = app.getCurrentPage()->id();
	Utility::writeRenderableProperties(pTable, ostr);
	static const std::string afterEdit("afteredit");
	static const std::string cellClicked("cellclick");
	std::map<std::string, std::string> addParams;
	addParams.insert(std::make_pair(Table::FIELD_COL, "+obj.column"));
	addParams.insert(std::make_pair(Table::FIELD_ROW, "+obj.row"));
	addParams.insert(std::make_pair(Table::FIELD_VAL, "+obj.value"));
	addParams.insert(std::make_pair(RequestHandler::KEY_EVID, Table::EV_CELLVALUECHANGED));
	JavaScriptEvent<int> ev;
	ev.setJSDelegates(pTable->cellValueChanged.jsDelegates());
	ev.add(jsDelegate("function(obj){obj.grid.getStore().commitChanges();}"));
	ostr << ",listeners:{";
	Utility::writeJSEventPlusServerCallback(ostr, afterEdit, ev.jsDelegates(), addParams, pTable->cellValueChanged.hasLocalHandlers());
	
	//cellclick : ( Grid this, Number rowIndex, Number columnIndex, Ext.EventObject e )
	//hm, more than one param in the eventhanlder of cellclick, writeJSEvent creates a fucntion(obj) wrapper
	//FIXME: no support for custom javascript yet
	addParams.clear();
	addParams.insert(std::make_pair(Table::FIELD_COL, "+columnIndex"));
	addParams.insert(std::make_pair(Table::FIELD_ROW, "+rowIndex"));
	addParams.insert(std::make_pair(RequestHandler::KEY_EVID, Table::EV_CELLCLICKED));
	ostr << ",";
	Utility::writeServerCallback(ostr,cellClicked, "function(aGrid, rowIndex,columnIndex,e)",addParams, pTable->cellClicked.hasLocalHandlers());
	
	ostr << "},"; //close listeners
	
	renderColumns(pTable, context, ostr);
	ostr << ",clicksToEdit:1,stripeRows:true";
	if (pTable->getWidth() > 0)
		ostr << ",width:" << pTable->getWidth();
	if (pTable->getHeight() > 0)
		ostr << ",height:" << pTable->getHeight();
	ostr << ",store:";
	renderStore(pTable, ostr);
	WebApplication::instance().registerAjaxProcessor(Poco::NumberFormatter::format(id), const_cast<Table*>(pTable));
}


void TableRenderer::renderColumns(const Table* pTable, const RenderContext& context, std::ostream& ostr)
{
	
	//columns: [...]
	ostr << "columns:[";
	const Table::TableColumns& columns = pTable->getColumns();
	Table::TableColumns::const_iterator it = columns.begin();
	int i = 0;
	for (; it != columns.end(); ++it, ++i)
	{
		if (i != 0)
			ostr << ",";

		renderColumn(pTable, *(*it), i, context, ostr);
	}
	ostr << "]";
}


void TableRenderer::renderColumn(const Table* pTable, const TableColumn& tc, int idx, const RenderContext& context, std::ostream& ostr)
{
	static LookAndFeel& laf = Utility::getDefaultRenderers();

	// {id:'company', header: "Company", width: 200, sortable: true, dataIndex: 'company'}
	// {header: "Last Updated", width: 135, sortable: true, renderer: Ext.util.Format.dateRenderer('m/d/Y'), dataIndex: 'lastChange'}
	ostr << "{";
	std::string hdr(Utility::safe(tc.getHeader()));

	ostr << "header:'" << hdr << "',dataIndex:'" << idx << "'";

	if (tc.getWidth() > 0)
		ostr << ",width:" << tc.getWidth();
	if (tc.isSortable())
		ostr << ",sortable:true";
	
	static TableCellHandlerFactory& fty = TableCellHandlerFactory::instance();

	if (tc.getCell())
	{
		AbstractTableCellHandler::Ptr pHandler = fty.factory(tc.getCell());
		if (tc.getCell()->isEditable())
			ostr << ",editable:true";
		if (tc.getCell()->isEditable() && pHandler->useEditor())
		{
			ostr << ",editor:";
			tc.getCell()->renderHead(context, ostr);
		}
		if (pHandler->useRenderer())
		{
			ostr << ",renderer:";
			pHandler->writeDynamicData(ostr);
		}
		
	}

	ostr << "}";
}


void TableRenderer::renderDataModel(const Table* pTable, std::ostream& ostr)
{
	//[
    //[    ['3m Co',71.72,0.02,0.03,'9/1 12:00am'],
    //[    ['Alcoa Inc',29.01,0.42,1.47,'9/1 12:00am']
	//]
	const TableModel& tm = pTable->getModel();
	const Table::TableColumns& tc = pTable->getColumns();

	poco_assert_dbg (tc.size() == tm.getColumnCount());

	std::size_t colCnt = tm.getColumnCount();
	std::size_t rowCnt = tm.getRowCount();
	ostr << "[";
	for (std::size_t row = 0; row < rowCnt; ++row)
	{
		if (row != 0)
			ostr << ",[";
		else
			ostr << "[";
		for (std::size_t col = 0; col < colCnt; ++col)
		{
			if (col != 0)
				ostr << ",";

			// how do we distinguish if we want to write something as text or GUIElement?
			// Example: Checkbutton can be written as text "true"/"false" or as a CheckButton
			// we use the Cell: if we have a Cell set -> complex Type otherwise text
			// -> already handled by the renderer!
			const Poco::Any& aVal = tm.getValue(row, col);
			
			if (aVal.empty())
				ostr << "''";
			else
			{
				//FIXME: we have no type nfo at all, assume string for everything
				bool isString = (typeid(std::string) == aVal.type());
				Cell::Ptr ptrCell = tc[col]->getCell();
				if (isString)
					ostr << "'" << RefAnyCast<std::string>(aVal) << "'";
				else if (ptrCell)
				{
					//date must be written as string
					if (typeid(Poco::DateTime) == aVal.type())
						ostr << "'" << tc[col]->getCell()->getFormatter()->format(aVal) << "'";
					else
						ostr  << tc[col]->getCell()->getFormatter()->format(aVal);
				}
				else
					; //FIXME: 
			}
		}
		ostr << "]";
	}
	ostr << "]";
}


void TableRenderer::renderStore(const Table* pTable, std::ostream& ostr)
{
	//new Ext.data.SimpleStore({
	//	fields: [
	//	   {name: 'company'},
	//	   {name: 'price', type: 'float'},
	//	   {name: 'change', type: 'float'},
	//	   {name: 'pctChange', type: 'float'},
	//	   {name: 'lastChange', type: 'date', dateFormat: 'n/j h:ia'}
	//	],
	//	data: [...]
	//});

	// we don't know the type, we just have a formatter, the name is always the idx!
	// we use the formatter later to set a renderer for a different type than string
	const Table::TableColumns& columns = pTable->getColumns();
	ostr << "new Ext.data.SimpleStore({fields:[";
	Table::TableColumns::const_iterator it = columns.begin();
	int i = 0;
	for (; it != columns.end(); ++it, ++i)
	{
		if (i != 0)
			ostr << ",";
		ostr << "{name:'" << i << "'}";
	}
	ostr << "],"; // close fields
	//Write data
	ostr << "data:";
	renderDataModel(pTable, ostr);
	ostr << "})";
}



} } } // namespace Poco::WebWidgets::ExtJS
