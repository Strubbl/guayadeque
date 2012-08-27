// -------------------------------------------------------------------------------- //
//	Copyright (C) 2008-2012 J.Rios
//	anonbeat@gmail.com
//
//    This Program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 3, or (at your option)
//    any later version.
//
//    This Program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; see the file LICENSE.  If not, write to
//    the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
//    http://www.gnu.org/copyleft/gpl.html
//
// -------------------------------------------------------------------------------- //
#include "RadioPanel.h"

#include "Accelerators.h"
#include "AuiDockArt.h"
#include "Commands.h"
#include "Config.h"
#include "Images.h"
#include "LabelEditor.h"
#include "MainFrame.h"
#include "PlayListFile.h"
#include "Preferences.h"
#include "RadioGenreEditor.h"
#include "RadioEditor.h"
#include "StatusBar.h"
#include "Settings.h"
#include "TagInfo.h"
#include "Utils.h"

#include <wx/wfstream.h>
#include <wx/treectrl.h>
#include <wx/tokenzr.h>
#include <wx/xml/xml.h>

#define guRADIO_TIMER_TEXTSEARCH        1
#define guRADIO_TIMER_TEXTSEARCH_VALUE  500

// -------------------------------------------------------------------------------- //
// guShoutcastItemData
// -------------------------------------------------------------------------------- //
class guShoutcastItemData : public wxTreeItemData
{
  private :
    int         m_Id;
    int         m_Source;
    wxString    m_Name;
    int         m_Flags;

  public :
    guShoutcastItemData( const int id, const int source, const wxString &name, int flags )
    {
        m_Id = id;
        m_Source = source;
        m_Name = name;
        m_Flags = flags;
    }

    int         GetId( void ) { return m_Id; }
    void        SetId( int id ) { m_Id = id; }
    int         GetSource( void ) { return m_Source; }
    void        SetSource( int source ) { m_Source = source; }
    int         GetFlags( void ) { return m_Flags; }
    void        SetFlags( int flags ) { m_Flags = flags; }
    wxString    GetName( void ) { return m_Name; }
    void        SetName( const wxString &name ) { m_Name = name; }

};

// -------------------------------------------------------------------------------- //
guShoutcastSearch::guShoutcastSearch( wxWindow * parent, guShoutcastItemData * itemdata ) :
    wxDialog( parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 435,237 ), wxDEFAULT_DIALOG_STYLE )
{
    m_ItemData = itemdata;
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* MainSizer;
	MainSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* LabelSizer;
	LabelSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _(" Radio Search ") ), wxVERTICAL );

	wxBoxSizer* SearchSizer;
	SearchSizer = new wxBoxSizer( wxHORIZONTAL );

	wxStaticText * SearchLabel = new wxStaticText( this, wxID_ANY, _("Search:"), wxDefaultPosition, wxDefaultSize, 0 );
	SearchLabel->Wrap( -1 );
	SearchSizer->Add( SearchLabel, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_SearchTextCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SearchTextCtrl->SetValue( m_ItemData->GetName() );
	SearchSizer->Add( m_SearchTextCtrl, 1, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	LabelSizer->Add( SearchSizer, 0, wxEXPAND, 5 );

	m_SearchPlayChkBox = new wxCheckBox( this, wxID_ANY, _("Search in now playing info"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SearchPlayChkBox->SetValue( m_ItemData->GetFlags() & guRADIO_SEARCH_FLAG_NOWPLAYING );
	LabelSizer->Add( m_SearchPlayChkBox, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_SearchGenreChkBox = new wxCheckBox( this, wxID_ANY, _("Search in genre names"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SearchGenreChkBox->SetValue( m_ItemData->GetFlags() & guRADIO_SEARCH_FLAG_GENRE );
	LabelSizer->Add( m_SearchGenreChkBox, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_SearchNameChkBox = new wxCheckBox( this, wxID_ANY, _("Search in station names"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SearchNameChkBox->SetValue( m_ItemData->GetFlags() & guRADIO_SEARCH_FLAG_STATION );
	LabelSizer->Add( m_SearchNameChkBox, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_AllGenresChkBox = new wxCheckBox( this, wxID_ANY, _("Include results from all genres"), wxDefaultPosition, wxDefaultSize, 0 );
	m_AllGenresChkBox->SetValue( m_ItemData->GetFlags() & guRADIO_SEARCH_FLAG_ALLGENRES );
	LabelSizer->Add( m_AllGenresChkBox, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	MainSizer->Add( LabelSizer, 1, wxEXPAND|wxALL, 5 );

	wxStdDialogButtonSizer * ButtonsSizer = new wxStdDialogButtonSizer();
	wxButton * ButtonsSizerOK = new wxButton( this, wxID_OK );
	ButtonsSizer->AddButton( ButtonsSizerOK );
	wxButton * ButtonsSizerCancel = new wxButton( this, wxID_CANCEL );
	ButtonsSizer->AddButton( ButtonsSizerCancel );
	ButtonsSizer->SetAffirmativeButton( ButtonsSizerOK );
	ButtonsSizer->SetCancelButton( ButtonsSizerCancel );
	ButtonsSizer->Realize();
	MainSizer->Add( ButtonsSizer, 0, wxEXPAND|wxBOTTOM|wxRIGHT, 5 );

	this->SetSizer( MainSizer );
	this->Layout();

	ButtonsSizerOK->SetDefault();

	Connect( wxID_OK, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( guShoutcastSearch::OnOkButton ) );

	m_SearchTextCtrl->SetFocus();
}

// -------------------------------------------------------------------------------- //
guShoutcastSearch::~guShoutcastSearch()
{
	Disconnect( wxID_OK, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( guShoutcastSearch::OnOkButton ) );
}

// -------------------------------------------------------------------------------- //
void guShoutcastSearch::OnOkButton( wxCommandEvent &event )
{
    m_ItemData->SetName( m_SearchTextCtrl->GetValue() );

    int flags = guRADIO_SEARCH_FLAG_NONE;

    if( m_SearchPlayChkBox->IsChecked() )
        flags |= guRADIO_SEARCH_FLAG_NOWPLAYING;

    if( m_SearchGenreChkBox->IsChecked() )
        flags |= guRADIO_SEARCH_FLAG_GENRE;

    if( m_SearchNameChkBox->IsChecked() )
        flags |= guRADIO_SEARCH_FLAG_STATION;

    if( m_AllGenresChkBox->IsChecked() )
        flags |= guRADIO_SEARCH_FLAG_ALLGENRES;

    m_ItemData->SetFlags( flags );

    event.Skip();
}




// -------------------------------------------------------------------------------- //
// guRadioGenreTreeCtrl
// -------------------------------------------------------------------------------- //
class guRadioGenreTreeCtrl : public wxTreeCtrl
{
  private :
    guDbRadios *    m_Db;
    wxImageList *   m_ImageList;
    wxTreeItemId    m_RootId;
    wxTreeItemId    m_ManualId;
    wxTreeItemId    m_ShoutcastId;
    wxTreeItemId    m_ShoutcastGenreId;
    wxTreeItemId    m_ShoutcastSearchsId;

    void            OnContextMenu( wxTreeEvent &event );
    void            OnRadioGenreAdd( wxCommandEvent &event );
    void            OnRadioGenreEdit( wxCommandEvent &event );
    void            OnRadioGenreDelete( wxCommandEvent &event );
    void            OnKeyDown( wxKeyEvent &event );

    void            OnConfigUpdated( wxCommandEvent &event );
    void            CreateAcceleratorTable( void );

  public :
    guRadioGenreTreeCtrl( wxWindow * parent, guDbRadios * db );
    ~guRadioGenreTreeCtrl();

    void            ReloadItems( void );
    wxTreeItemId *  GetShoutcastId( void ) { return &m_ShoutcastId; };
    wxTreeItemId *  GetShoutcastGenreId( void ) { return &m_ShoutcastGenreId; };
    wxTreeItemId *  GetShoutcastSearchId( void ) { return &m_ShoutcastSearchsId; };
    wxTreeItemId *  GetManualId( void ) { return &m_ManualId; };
    wxTreeItemId    GetItemId( wxTreeItemId * itemid, const int id );

};

// -------------------------------------------------------------------------------- //
// guRadioLabelListBox
// -------------------------------------------------------------------------------- //
class guRadioLabelListBox : public guAccelListBox
{
  protected :

    virtual void    GetItemsList( void );
    virtual void    CreateContextMenu( wxMenu * Menu ) const;
    void            AddLabel( wxCommandEvent &event );
    void            DelLabel( wxCommandEvent &event );
    void            EditLabel( wxCommandEvent &event );

    virtual void    CreateAcceleratorTable( void );

    public :

      guRadioLabelListBox( wxWindow * parent, guDbRadios * NewDb, wxString Label );
      ~guRadioLabelListBox();

};

// -------------------------------------------------------------------------------- //
// guUpdateRadiosThread
// -------------------------------------------------------------------------------- //
class guUpdateRadiosThread : public wxThread
{
  private:
    guDbRadios *    m_Db;
    guRadioPanel *  m_RadioPanel;
    int             m_GaugeId;
    wxArrayInt      m_Ids;
    int             m_Source;

    void            CheckRadioStationsFilters( const int flags, const wxString &text, guRadioStations &stations );

  public:
    guUpdateRadiosThread( guDbRadios * db, guRadioPanel * radiopanel,
                                const wxArrayInt &ids, const int source, int gaugeid = wxNOT_FOUND );

    ~guUpdateRadiosThread(){};

    virtual ExitCode Entry();
};

// -------------------------------------------------------------------------------- //
// guRadioStationListBox
// -------------------------------------------------------------------------------- //
class guRadioStationListBox : public guListView
{
  protected :
    guDbRadios *                m_Db;
    guRadioStations             m_Radios;
    int                         m_StationsOrder;
    bool                        m_StationsOrderDesc;

    virtual void                CreateContextMenu( wxMenu * Menu ) const;
    virtual wxString            OnGetItemText( const int row, const int column ) const;
    virtual void                GetItemsList( void );

    virtual wxArrayString       GetColumnNames( void );

    void                        OnConfigUpdated( wxCommandEvent &event );
    void                        CreateAcceleratorTable();

  public :
    guRadioStationListBox( wxWindow * parent, guDbRadios * NewDb );
    ~guRadioStationListBox();

    virtual void                ReloadItems( bool reset = true );

    virtual int inline          GetItemId( const int row ) const;
    virtual wxString inline     GetItemName( const int row ) const;

    void                        SetStationsOrder( int order );
    bool                        GetSelected( guRadioStation * radiostation ) const;
};




// -------------------------------------------------------------------------------- //
// guRadioGenreTreeCtrl
// -------------------------------------------------------------------------------- //
guRadioGenreTreeCtrl::guRadioGenreTreeCtrl( wxWindow * parent, guDbRadios * db ) :
    wxTreeCtrl( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxTR_DEFAULT_STYLE|wxTR_SINGLE|wxTR_HIDE_ROOT|wxTR_FULL_ROW_HIGHLIGHT )
{
    m_Db = db;

    guConfig * Config = ( guConfig * ) guConfig::Get();
    Config->RegisterObject( this );

    m_ImageList = new wxImageList();
    m_ImageList->Add( guImage( guIMAGE_INDEX_tiny_shoutcast ) );
    m_ImageList->Add( wxBitmap( guImage( guIMAGE_INDEX_tiny_net_radio ) ) );
    m_ImageList->Add( wxBitmap( guImage( guIMAGE_INDEX_tiny_search ) ) );

    AssignImageList( m_ImageList );

    m_RootId   = AddRoot( wxT( "Radios" ), -1, -1, NULL );
    m_ShoutcastId = AppendItem( m_RootId, _( "Shoutcast" ), 0, 0, NULL );
    m_ShoutcastGenreId = AppendItem( m_ShoutcastId, _( "Genre" ), 0, 0, NULL );
    m_ShoutcastSearchsId = AppendItem( m_ShoutcastId, _( "Searches" ), 2, 2, NULL );
    m_ManualId = AppendItem( m_RootId, _( "User Defined" ), 1, 1, NULL );

    SetIndent( 5 );

    Connect( wxEVT_COMMAND_TREE_ITEM_MENU, wxTreeEventHandler( guRadioGenreTreeCtrl::OnContextMenu ), NULL, this );
    Connect( ID_RADIO_GENRE_ADD, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioGenreTreeCtrl::OnRadioGenreAdd ) );
    Connect( ID_RADIO_GENRE_EDIT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioGenreTreeCtrl::OnRadioGenreEdit ) );
    Connect( ID_RADIO_GENRE_DELETE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioGenreTreeCtrl::OnRadioGenreDelete ) );
    Connect( wxEVT_KEY_DOWN, wxKeyEventHandler( guRadioGenreTreeCtrl::OnKeyDown ), NULL, this );

    Connect( ID_CONFIG_UPDATED, guConfigUpdatedEvent, wxCommandEventHandler( guRadioGenreTreeCtrl::OnConfigUpdated ), NULL, this );

    CreateAcceleratorTable();

    ReloadItems();
}

// -------------------------------------------------------------------------------- //
guRadioGenreTreeCtrl::~guRadioGenreTreeCtrl()
{
    guConfig * Config = ( guConfig * ) guConfig::Get();
    Config->UnRegisterObject( this );

    Disconnect( ID_RADIO_GENRE_ADD, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioGenreTreeCtrl::OnRadioGenreAdd ) );
    Disconnect( ID_RADIO_GENRE_EDIT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioGenreTreeCtrl::OnRadioGenreEdit ) );
    Disconnect( ID_RADIO_GENRE_DELETE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioGenreTreeCtrl::OnRadioGenreDelete ) );
    Disconnect( wxEVT_COMMAND_TREE_ITEM_MENU, wxTreeEventHandler( guRadioGenreTreeCtrl::OnContextMenu ), NULL, this );

    Disconnect( wxEVT_KEY_DOWN, wxKeyEventHandler( guRadioGenreTreeCtrl::OnKeyDown ), NULL, this );
}

// -------------------------------------------------------------------------------- //
void guRadioGenreTreeCtrl::OnConfigUpdated( wxCommandEvent &event )
{
    int Flags = event.GetInt();
    if( Flags & guPREFERENCE_PAGE_FLAG_ACCELERATORS )
    {
        CreateAcceleratorTable();
    }
}

// -------------------------------------------------------------------------------- //
void guRadioGenreTreeCtrl::CreateAcceleratorTable( void )
{
    wxAcceleratorTable AccelTable;
    wxArrayInt AliasAccelCmds;
    wxArrayInt RealAccelCmds;

    AliasAccelCmds.Add( ID_PLAYER_PLAYLIST_SEARCH );

    RealAccelCmds.Add( ID_RADIO_SEARCH );

    if( guAccelDoAcceleratorTable( AliasAccelCmds, RealAccelCmds, AccelTable ) )
    {
        SetAcceleratorTable( AccelTable );
    }
}

// -------------------------------------------------------------------------------- //
void guRadioGenreTreeCtrl::ReloadItems( void )
{
    DeleteChildren( m_ShoutcastGenreId );

    guListItems RadioGenres;
    m_Db->GetRadioGenres( guRADIO_SOURCE_GENRE, &RadioGenres );

    int index;
    int count = RadioGenres.Count();
    for( index = 0; index < count; index++ )
    {
        AppendItem( m_ShoutcastGenreId, RadioGenres[ index ].m_Name, -1, -1,
            new guShoutcastItemData( RadioGenres[ index ].m_Id, guRADIO_SOURCE_GENRE, RadioGenres[ index ].m_Name, 0 ) );
    }

    DeleteChildren( m_ShoutcastSearchsId );
    guListItems RadioSearchs;
    wxArrayInt RadioFlags;
    m_Db->GetRadioGenres( guRADIO_SOURCE_SEARCH, &RadioSearchs, true, &RadioFlags );
    count = RadioSearchs.Count();
    for( index = 0; index < count; index++ )
    {
        AppendItem( m_ShoutcastSearchsId, RadioSearchs[ index ].m_Name, -1, -1,
            new guShoutcastItemData( RadioSearchs[ index ].m_Id, guRADIO_SOURCE_SEARCH, RadioSearchs[ index ].m_Name, RadioFlags[ index ] ) );
    }
}

// -------------------------------------------------------------------------------- //
void guRadioGenreTreeCtrl::OnContextMenu( wxTreeEvent &event )
{
    wxMenu Menu;
    wxMenuItem * MenuItem;

    wxPoint Point = event.GetPoint();

    wxTreeItemId ItemId = event.GetItem();
    guShoutcastItemData * ItemData = ( guShoutcastItemData * ) GetItemData( ItemId );


    if( ( ItemData && ( ItemData->GetSource() == guRADIO_SOURCE_GENRE ) ) || ( ItemId == * GetShoutcastGenreId() ) )
    {
        MenuItem = new wxMenuItem( &Menu, ID_RADIO_GENRE_ADD, _( "Add Genre" ), _( "Create a new genre" ) );
        MenuItem->SetBitmap( guImage( guIMAGE_INDEX_tiny_add ) );
        Menu.Append( MenuItem );

        if( ItemData )
        {
            MenuItem = new wxMenuItem( &Menu, ID_RADIO_GENRE_EDIT, _( "Edit genre" ), _( "Change the selected genre" ) );
            MenuItem->SetBitmap( guImage( guIMAGE_INDEX_tiny_edit ) );
            Menu.Append( MenuItem );

            MenuItem = new wxMenuItem( &Menu, ID_RADIO_GENRE_DELETE, _( "Delete genre" ), _( "Delete the selected search" ) );
            MenuItem->SetBitmap( guImage( guIMAGE_INDEX_tiny_edit_clear ) );
            Menu.Append( MenuItem );
        }

        Menu.AppendSeparator();

        MenuItem = new wxMenuItem( &Menu, ID_RADIO_DOUPDATE, _( "Update Radio Stations" ), _( "Update the radio station lists" ) );
        MenuItem->SetBitmap( guImage( guIMAGE_INDEX_tiny_reload ) );
        Menu.Append( MenuItem );
    }
    else if( ( ItemData && ( ItemData->GetSource() == guRADIO_SOURCE_SEARCH ) ) || ( ItemId == * GetShoutcastSearchId() ) )
    {
        MenuItem = new wxMenuItem( &Menu, ID_RADIO_SEARCH_ADD, _( "Add Search" ), _( "Create a new search" ) );
        MenuItem->SetBitmap( guImage( guIMAGE_INDEX_tiny_add ) );
        Menu.Append( MenuItem );

        if( ItemData )
        {
            MenuItem = new wxMenuItem( &Menu, ID_RADIO_SEARCH_EDIT, _( "Edit search" ), _( "Change the selected search" ) );
            MenuItem->SetBitmap( guImage( guIMAGE_INDEX_tiny_edit ) );
            Menu.Append( MenuItem );

            MenuItem = new wxMenuItem( &Menu, ID_RADIO_SEARCH_DELETE, _( "Delete search" ), _( "Delete the selected search" ) );
            MenuItem->SetBitmap( guImage( guIMAGE_INDEX_tiny_edit_clear ) );
            Menu.Append( MenuItem );
        }

        Menu.AppendSeparator();

        MenuItem = new wxMenuItem( &Menu, ID_RADIO_DOUPDATE, _( "Update Radio Stations" ), _( "Update the radio station lists" ) );
        MenuItem->SetBitmap( guImage( guIMAGE_INDEX_tiny_reload ) );
        Menu.Append( MenuItem );
    }
    else if( ItemId == * GetManualId() )
    {
        MenuItem = new wxMenuItem( &Menu, ID_RADIO_USER_ADD, _( "Add Radio" ), _( "Create a new radio" ) );
        MenuItem->SetBitmap( guImage( guIMAGE_INDEX_tiny_add ) );
        Menu.Append( MenuItem );

        Menu.AppendSeparator();

        MenuItem = new wxMenuItem( &Menu, ID_RADIO_USER_IMPORT, _( "Import" ), _( "Import the radio stations" ) );
        MenuItem->SetBitmap( guImage( guIMAGE_INDEX_tiny_add ) );
        Menu.Append( MenuItem );

        MenuItem = new wxMenuItem( &Menu, ID_RADIO_USER_EXPORT, _( "Export" ), _( "Export all the radio stations" ) );
        MenuItem->SetBitmap( guImage( guIMAGE_INDEX_tiny_doc_save ) );
        Menu.Append( MenuItem );
    }
    else
    {
        MenuItem = new wxMenuItem( &Menu, ID_RADIO_DOUPDATE, _( "Update Radio Stations" ), _( "Update the radio station lists" ) );
        MenuItem->SetBitmap( guImage( guIMAGE_INDEX_tiny_reload ) );
        Menu.Append( MenuItem );
    }

    PopupMenu( &Menu, Point );
    event.Skip();
}

// -------------------------------------------------------------------------------- //
void guRadioGenreTreeCtrl::OnRadioGenreAdd( wxCommandEvent &event )
{
    int Index;
    int Count;
    guRadioGenreEditor * RadioGenreEditor = new guRadioGenreEditor( this, m_Db );
    if( RadioGenreEditor )
    {
        bool NeedReload = false;
        //
        if( RadioGenreEditor->ShowModal() == wxID_OK )
        {
            wxArrayString NewGenres;
            wxArrayInt DelGenres;
            RadioGenreEditor->GetGenres( NewGenres, DelGenres );
            if( ( Count = NewGenres.Count() ) )
            {
                //
                for( Index = 0; Index < Count; Index++ )
                {
                    m_Db->AddRadioGenre( NewGenres[ Index ], guRADIO_SOURCE_GENRE, guRADIO_SEARCH_FLAG_NONE );
                }
                NeedReload = true;
            }

            if( ( Count = DelGenres.Count() ) )
            {
                for( Index = 0; Index < Count; Index++ )
                {
                    m_Db->DelRadioGenre( DelGenres[ Index ] );
                }
                NeedReload = true;
            }

            if( NeedReload )
                ReloadItems();
        }
        //
        RadioGenreEditor->Destroy();
    }
}

// -------------------------------------------------------------------------------- //
void guRadioGenreTreeCtrl::OnRadioGenreEdit( wxCommandEvent &event )
{
    wxTreeItemId ItemId = GetSelection();

    if( ItemId.IsOk() )
    {
        guShoutcastItemData * RadioGenreData = ( guShoutcastItemData * ) GetItemData( ItemId );

        if( RadioGenreData )
        {
            // Get the Index of the First Selected Item
            wxTextEntryDialog * EntryDialog = new wxTextEntryDialog( this, _( "Genre Name: " ), _( "Enter the new Genre Name" ), RadioGenreData->GetName() );
            if( EntryDialog->ShowModal() == wxID_OK )
            {
                m_Db->SetRadioGenre( RadioGenreData->GetId(), EntryDialog->GetValue() );
                ReloadItems();
            }
            EntryDialog->Destroy();
        }
    }
}

// -------------------------------------------------------------------------------- //
void guRadioGenreTreeCtrl::OnRadioGenreDelete( wxCommandEvent &event )
{
    wxArrayTreeItemIds ItemIds;
    int index;
    int count;

    if( ( count = GetSelections( ItemIds ) ) )
    {
        if( wxMessageBox( _( "Are you sure to delete the selected radio genres?" ),
                          _( "Confirm" ),
                          wxICON_QUESTION|wxYES_NO|wxNO_DEFAULT, this ) == wxYES )
        {
            guShoutcastItemData * RadioGenreData;
            for( index = 0; index < count; index++ )
            {
                RadioGenreData = ( guShoutcastItemData * ) GetItemData( ItemIds[ index ] );
                if( RadioGenreData )
                {
                    m_Db->DelRadioGenre( RadioGenreData->GetId() );
                }
            }
            ReloadItems();
        }
    }
}

// -------------------------------------------------------------------------------- //
wxTreeItemId guRadioGenreTreeCtrl::GetItemId( wxTreeItemId * itemid, const int id )
{
    wxTreeItemIdValue Cookie;
    wxTreeItemId CurItem = GetFirstChild( * itemid, Cookie );
    while( CurItem.IsOk() )
    {
        guShoutcastItemData * ItemData = ( guShoutcastItemData * ) GetItemData( CurItem );
        if( ItemData )
        {
            if( ItemData->GetId() == id )
                return CurItem;
        }
        else
        {
            wxTreeItemId ChildItem = GetItemId( &CurItem, id );
            if( ChildItem.IsOk() )
                return ChildItem;
        }
        CurItem = GetNextChild( * itemid, Cookie );
    }
    return CurItem;
}

// -------------------------------------------------------------------------------- //
void guRadioGenreTreeCtrl::OnKeyDown( wxKeyEvent &event )
{
    if( event.GetKeyCode() == WXK_DELETE )
    {
        wxCommandEvent CmdEvent( wxEVT_COMMAND_MENU_SELECTED, ID_RADIO_GENRE_DELETE );
        wxPostEvent( this, CmdEvent );
        return;
    }
    event.Skip();
}





// -------------------------------------------------------------------------------- //
// guRadioStationListBox
// -------------------------------------------------------------------------------- //
guRadioStationListBox::guRadioStationListBox( wxWindow * parent, guDbRadios * db ) :
    guListView( parent, wxLB_SINGLE | guLISTVIEW_COLUMN_SELECT|guLISTVIEW_COLUMN_SORTING )
{
    m_Db = db;

    guConfig * Config = ( guConfig * ) guConfig::Get();
    Config->RegisterObject( this );

    m_StationsOrder = Config->ReadNum( wxT( "StationsOrder" ), 0, wxT( "radios" ) );
    m_StationsOrderDesc = Config->ReadNum( wxT( "StationsOrderDesc" ), false, wxT( "radios" ) );;

    wxArrayString ColumnNames = GetColumnNames();
    // Create the Columns
    int ColId;
    wxString ColName;
    int index;
    int count = ColumnNames.Count();
    for( index = 0; index < count; index++ )
    {
        ColId = Config->ReadNum( wxString::Format( wxT( "id%u" ), index ), index, wxT( "radios/columns/ids" ) );

        ColName = ColumnNames[ ColId ];

        ColName += ( ( ColId == m_StationsOrder ) ? ( m_StationsOrderDesc ? wxT( " ▼" ) : wxT( " ▲" ) ) : wxEmptyString );

        guListViewColumn * Column = new guListViewColumn(
            ColName,
            ColId,
            Config->ReadNum( wxString::Format( wxT( "width%u" ), index ), 80, wxT( "radios/columns/widths" ) ),
            Config->ReadBool( wxString::Format( wxT( "show%u" ), index ), true, wxT( "radios/columns/shows" ) )
            );
        InsertColumn( Column );
    }

    Connect( ID_CONFIG_UPDATED, guConfigUpdatedEvent, wxCommandEventHandler( guRadioStationListBox::OnConfigUpdated ), NULL, this );

    CreateAcceleratorTable();

    ReloadItems();
}


// -------------------------------------------------------------------------------- //
guRadioStationListBox::~guRadioStationListBox()
{
    guConfig * Config = ( guConfig * ) guConfig::Get();
    Config->UnRegisterObject( this );

    //int ColId;
    int index;
    int count = guRADIOSTATIONS_COLUMN_COUNT;
    for( index = 0; index < count; index++ )
    {
        Config->WriteNum( wxString::Format( wxT( "id%u" ), index ), ( * m_Columns )[ index ].m_Id, wxT( "radios/columns/ids" ) );
        Config->WriteNum( wxString::Format( wxT( "width%u" ), index ), ( * m_Columns )[ index ].m_Width, wxT( "radios/columns/widths" ) );
        Config->WriteBool( wxString::Format( wxT( "show%u" ), index ), ( * m_Columns )[ index ].m_Enabled, wxT( "radios/columns/shows" ) );
    }

    Config->WriteNum( wxT( "StationsOrder" ), m_StationsOrder, wxT( "radios" ) );
    Config->WriteBool( wxT( "StationsOrderDesc" ), m_StationsOrderDesc, wxT( "radios" ) );;

    Disconnect( ID_CONFIG_UPDATED, guConfigUpdatedEvent, wxCommandEventHandler( guRadioStationListBox::OnConfigUpdated ), NULL, this );
}

// -------------------------------------------------------------------------------- //
void guRadioStationListBox::OnConfigUpdated( wxCommandEvent &event )
{
    int Flags = event.GetInt();
    if( Flags & guPREFERENCE_PAGE_FLAG_ACCELERATORS )
    {
        CreateAcceleratorTable();
    }
}

// -------------------------------------------------------------------------------- //
void guRadioStationListBox::CreateAcceleratorTable( void )
{
    wxAcceleratorTable AccelTable;
    wxArrayInt AliasAccelCmds;
    wxArrayInt RealAccelCmds;

    AliasAccelCmds.Add( ID_PLAYER_PLAYLIST_EDITTRACKS );
    AliasAccelCmds.Add( ID_PLAYER_PLAYLIST_EDITLABELS );
    AliasAccelCmds.Add( ID_TRACKS_PLAY );
    AliasAccelCmds.Add( ID_TRACKS_ENQUEUE_AFTER_ALL );
    AliasAccelCmds.Add( ID_TRACKS_ENQUEUE_AFTER_TRACK );
    AliasAccelCmds.Add( ID_TRACKS_ENQUEUE_AFTER_ALBUM );
    AliasAccelCmds.Add( ID_TRACKS_ENQUEUE_AFTER_ARTIST );
    AliasAccelCmds.Add( ID_PLAYER_PLAYLIST_SEARCH );

    RealAccelCmds.Add( ID_RADIO_USER_EDIT );
    RealAccelCmds.Add( ID_RADIO_EDIT_LABELS );
    RealAccelCmds.Add( ID_RADIO_PLAY );
    RealAccelCmds.Add( ID_RADIO_ENQUEUE_AFTER_ALL );
    RealAccelCmds.Add( ID_RADIO_ENQUEUE_AFTER_TRACK );
    RealAccelCmds.Add( ID_RADIO_ENQUEUE_AFTER_ALBUM );
    RealAccelCmds.Add( ID_RADIO_ENQUEUE_AFTER_ARTIST );
    RealAccelCmds.Add( ID_RADIO_SEARCH );

    if( guAccelDoAcceleratorTable( AliasAccelCmds, RealAccelCmds, AccelTable ) )
    {
        SetAcceleratorTable( AccelTable );
    }
}

// -------------------------------------------------------------------------------- //
wxArrayString guRadioStationListBox::GetColumnNames( void )
{
    wxArrayString ColumnNames;
    ColumnNames.Add( _( "Name" ) );
    ColumnNames.Add( _( "BitRate" ) );
    ColumnNames.Add( _( "Listeners" ) );
    ColumnNames.Add( _( "Format" ) );
    ColumnNames.Add( _( "Now Playing" ) );
    return ColumnNames;
}

// -------------------------------------------------------------------------------- //
wxString guRadioStationListBox::OnGetItemText( const int row, const int col ) const
{
    guRadioStation * Radio;
    Radio = &m_Radios[ row ];
    switch( ( * m_Columns )[ col ].m_Id )
    {
        case guRADIOSTATIONS_COLUMN_NAME :
          return Radio->m_Name;
          break;

        case guRADIOSTATIONS_COLUMN_BITRATE :
          return wxString::Format( wxT( "%u" ), Radio->m_BitRate );
          break;

        case guRADIOSTATIONS_COLUMN_LISTENERS :
          return wxString::Format( wxT( "%u" ), Radio->m_Listeners );

        case guRADIOSTATIONS_COLUMN_TYPE :
          return Radio->m_Type;
          break;

        case guRADIOSTATIONS_COLUMN_NOWPLAYING :
          return Radio->m_NowPlaying;
          break;
}
    return wxEmptyString;
}


// -------------------------------------------------------------------------------- //
void guRadioStationListBox::GetItemsList( void )
{
    m_Db->GetRadioStations( &m_Radios );

    wxCommandEvent event( wxEVT_COMMAND_MENU_SELECTED, ID_MAINFRAME_UPDATE_SELINFO );
    AddPendingEvent( event );
}

// -------------------------------------------------------------------------------- //
void guRadioStationListBox::ReloadItems( bool reset )
{
    //
    wxArrayInt Selection;
    int FirstVisible = GetFirstVisibleLine();

    if( reset )
        SetSelection( -1 );
    else
        Selection = GetSelectedItems( false );

    m_Radios.Empty();

    GetItemsList();

    SetItemCount( m_Radios.Count() );

    if( !reset )
    {
      SetSelectedItems( Selection );
      ScrollToLine( FirstVisible );
    }
    RefreshAll();
}

// -------------------------------------------------------------------------------- //
void guRadioStationListBox::CreateContextMenu( wxMenu * Menu ) const
{
    wxMenuItem * MenuItem;
    int SelCount = GetSelectedCount();

    MenuItem = new wxMenuItem( Menu, ID_RADIO_USER_ADD, _( "Add Radio" ), _( "Add a radio" ) );
    MenuItem->SetBitmap( guImage( guIMAGE_INDEX_tiny_net_radio ) );
    Menu->Append( MenuItem );

    if( SelCount )
    {
        guRadioStation RadioStation;
        GetSelected( &RadioStation );

        if( RadioStation.m_Source == 1 )
        {
            MenuItem = new wxMenuItem( Menu, ID_RADIO_USER_EDIT,
                            wxString( _( "Edit Radio" ) ) + guAccelGetCommandKeyCodeString( ID_PLAYER_PLAYLIST_EDITTRACKS ),
                            _( "Change the selected radio" ) );
            MenuItem->SetBitmap( guImage( guIMAGE_INDEX_tiny_edit ) );
            Menu->Append( MenuItem );

            MenuItem = new wxMenuItem( Menu, ID_RADIO_USER_DEL, _( "Delete Radio" ), _( "Delete the selected radio" ) );
            MenuItem->SetBitmap( guImage( guIMAGE_INDEX_tiny_edit_clear ) );
            Menu->Append( MenuItem );
        }

        Menu->AppendSeparator();

        MenuItem = new wxMenuItem( Menu, ID_RADIO_PLAY,
                            wxString( _( "Play" ) ) + guAccelGetCommandKeyCodeString( ID_TRACKS_PLAY ),
                            _( "Play current selected songs" ) );
        MenuItem->SetBitmap( guImage( guIMAGE_INDEX_player_tiny_light_play ) );
        Menu->Append( MenuItem );

        MenuItem = new wxMenuItem( Menu, ID_RADIO_ENQUEUE_AFTER_ALL,
                            wxString( _( "Enqueue" ) ) + guAccelGetCommandKeyCodeString( ID_TRACKS_ENQUEUE_AFTER_ALL ),
                            _( "Add current selected songs to playlist" ) );
        MenuItem->SetBitmap( guImage( guIMAGE_INDEX_tiny_add ) );
        Menu->Append( MenuItem );

        wxMenu * EnqueueMenu = new wxMenu();

        MenuItem = new wxMenuItem( EnqueueMenu, ID_RADIO_ENQUEUE_AFTER_TRACK,
                                wxString( _( "Current Track" ) ) +  guAccelGetCommandKeyCodeString( ID_TRACKS_ENQUEUE_AFTER_TRACK ),
                                _( "Add current selected tracks to playlist after the current track" ) );
        MenuItem->SetBitmap( guImage( guIMAGE_INDEX_tiny_add ) );
        EnqueueMenu->Append( MenuItem );
        MenuItem->Enable( SelCount );

        MenuItem = new wxMenuItem( EnqueueMenu, ID_RADIO_ENQUEUE_AFTER_ALBUM,
                                wxString( _( "Current Album" ) ) +  guAccelGetCommandKeyCodeString( ID_TRACKS_ENQUEUE_AFTER_ALBUM ),
                                _( "Add current selected tracks to playlist after the current album" ) );
        MenuItem->SetBitmap( guImage( guIMAGE_INDEX_tiny_add ) );
        EnqueueMenu->Append( MenuItem );
        MenuItem->Enable( SelCount );

        MenuItem = new wxMenuItem( EnqueueMenu, ID_RADIO_ENQUEUE_AFTER_ARTIST,
                                wxString( _( "Current Artist" ) ) +  guAccelGetCommandKeyCodeString( ID_TRACKS_ENQUEUE_AFTER_ARTIST ),
                                _( "Add current selected tracks to playlist after the current artist" ) );
        MenuItem->SetBitmap( guImage( guIMAGE_INDEX_tiny_add ) );
        EnqueueMenu->Append( MenuItem );
        MenuItem->Enable( SelCount );

        Menu->Append( wxID_ANY, _( "Enqueue After" ), EnqueueMenu );

        Menu->AppendSeparator();

        MenuItem = new wxMenuItem( Menu, ID_RADIO_EDIT_LABELS,
                            wxString( _( "Edit Labels" ) ) + guAccelGetCommandKeyCodeString( ID_PLAYER_PLAYLIST_EDITLABELS ),
                            _( "Edit the labels assigned to the selected stations" ) );
        MenuItem->SetBitmap( guImage( guIMAGE_INDEX_tags ) );
        Menu->Append( MenuItem );
    }
}

// -------------------------------------------------------------------------------- //
int inline guRadioStationListBox::GetItemId( const int row ) const
{
    return m_Radios[ row ].m_Id;
}

// -------------------------------------------------------------------------------- //
wxString inline guRadioStationListBox::GetItemName( const int row ) const
{
    return m_Radios[ row ].m_Name;
}

// -------------------------------------------------------------------------------- //
void guRadioStationListBox::SetStationsOrder( int order )
{
    if( m_StationsOrder != order )
    {
        m_StationsOrder = order;
        m_StationsOrderDesc = ( order != 0 );
    }
    else
        m_StationsOrderDesc = !m_StationsOrderDesc;

    m_Db->SetRadioStationsOrder( m_StationsOrder );

    wxArrayString ColumnNames = GetColumnNames();
    int CurColId;
    int index;
    int count = ColumnNames.Count();
    for( index = 0; index < count; index++ )
    {
        CurColId = GetColumnId( index );
        SetColumnLabel( index,
            ColumnNames[ CurColId ]  + ( ( order == CurColId ) ? ( m_StationsOrderDesc ? wxT( " ▼" ) : wxT( " ▲" ) ) : wxEmptyString ) );
    }

    ReloadItems();


//    wxArrayString ColumnNames = m_SongListCtrl->GetColumnNames();
//    int CurColId;
//    int index;
//    int count = ColumnNames.Count();
//    for( index = 0; index < count; index++ )
//    {
//        CurColId = m_SongListCtrl->GetColumnId( index );
//        m_SongListCtrl->SetColumnLabel( index,
//            ColumnNames[ CurColId ]  + ( ( ColId == CurColId ) ? ( m_Db->GetSongsOrderDesc() ? wxT( " ▼" ) : wxT( " ▲" ) ) : wxEmptyString ) );
//    }
//
//    m_SongListCtrl->ReloadItems();
}

// -------------------------------------------------------------------------------- //
bool guRadioStationListBox::GetSelected( guRadioStation * radiostation ) const
{
    if( !m_Radios.Count() )
        return false;

    int Selected = GetSelection();
    if( Selected == wxNOT_FOUND )
    {
        Selected = 0;
    }
    radiostation->m_Id          = m_Radios[ Selected ].m_Id;
    radiostation->m_SCId        = m_Radios[ Selected ].m_SCId;
    radiostation->m_BitRate     = m_Radios[ Selected ].m_BitRate;
    radiostation->m_GenreId     = m_Radios[ Selected ].m_GenreId;
    radiostation->m_Source      = m_Radios[ Selected ].m_Source;
    radiostation->m_Link        = m_Radios[ Selected ].m_Link;
    radiostation->m_Listeners   = m_Radios[ Selected ].m_Listeners;
    radiostation->m_Name        = m_Radios[ Selected ].m_Name;
    radiostation->m_Type        = m_Radios[ Selected ].m_Type;
    return true;
}

// -------------------------------------------------------------------------------- //
// guRadioLabelListBox
// -------------------------------------------------------------------------------- //
guRadioLabelListBox::guRadioLabelListBox( wxWindow * parent, guDbRadios * db, wxString label ) :
    guAccelListBox( parent, ( guDbLibrary * ) db, label )
{
    Connect( ID_LABEL_ADD, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioLabelListBox::AddLabel ) );
    Connect( ID_LABEL_DELETE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioLabelListBox::DelLabel ) );
    Connect( ID_LABEL_EDIT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioLabelListBox::EditLabel ) );

    CreateAcceleratorTable();

    ReloadItems();
}

// -------------------------------------------------------------------------------- //
guRadioLabelListBox::~guRadioLabelListBox()
{
    Disconnect( ID_LABEL_ADD, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioLabelListBox::AddLabel ) );
    Disconnect( ID_LABEL_DELETE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioLabelListBox::DelLabel ) );
    Disconnect( ID_LABEL_EDIT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioLabelListBox::EditLabel ) );
}

// -------------------------------------------------------------------------------- //
void guRadioLabelListBox::GetItemsList( void )
{
    ( ( guDbRadios * ) m_Db )->GetRadioLabels( m_Items );
}

// -------------------------------------------------------------------------------- //
void guRadioLabelListBox::CreateContextMenu( wxMenu * Menu ) const
{
    wxMenuItem * MenuItem;

    MenuItem = new wxMenuItem( Menu, ID_LABEL_ADD, _( "Add Label" ), _( "Create a new label" ) );
    MenuItem->SetBitmap( guImage( guIMAGE_INDEX_doc_new ) );
    Menu->Append( MenuItem );

    if( GetSelectedCount() )
    {
        MenuItem = new wxMenuItem( Menu, ID_LABEL_EDIT, _( "Edit Label" ), _( "Change selected label" ) );
        MenuItem->SetBitmap( guImage( guIMAGE_INDEX_edit ) );
        Menu->Append( MenuItem );

        MenuItem = new wxMenuItem( Menu, ID_LABEL_DELETE, _( "Delete label" ), _( "Delete selected labels" ) );
        MenuItem->SetBitmap( guImage( guIMAGE_INDEX_edit_delete ) );
        Menu->Append( MenuItem );
    }
}

// -------------------------------------------------------------------------------- //
void guRadioLabelListBox::AddLabel( wxCommandEvent &event )
{
    //wxMessageBox( wxT( "AddLabel" ), wxT( "Information" ) );
    wxTextEntryDialog * EntryDialog = new wxTextEntryDialog( this, _( "Label Name: " ), _( "Please enter the label name" ) );
    if( EntryDialog->ShowModal() == wxID_OK )
    {
        //wxMessageBox( EntryDialog->GetValue(), wxT( "Entered..." ) );
        ( ( guDbRadios * ) m_Db )->AddRadioLabel( EntryDialog->GetValue() );
        ReloadItems();
    }
    EntryDialog->Destroy();
}

// -------------------------------------------------------------------------------- //
void guRadioLabelListBox::DelLabel( wxCommandEvent &event )
{
    wxArrayInt Selection = GetSelectedItems();
    int Count = Selection.Count();
    if( Count )
    {
        if( wxMessageBox( _( "Are you sure to delete the selected labels?" ),
                          _( "Confirm" ),
                          wxICON_QUESTION|wxYES_NO|wxNO_DEFAULT, this ) == wxYES )
        {
            for( int Index = 0; Index < Count; Index++ )
            {
                ( ( guDbRadios * ) m_Db )->DelRadioLabel( Selection[ Index ] );
            }
            ReloadItems();
        }
    }
}

// -------------------------------------------------------------------------------- //
void guRadioLabelListBox::EditLabel( wxCommandEvent &event )
{
    wxArrayInt Selection = GetSelectedItems();
    if( Selection.Count() )
    {
        // Get the Index of the First Selected Item
        unsigned long cookie;
        int item = GetFirstSelected( cookie );
        wxTextEntryDialog * EntryDialog = new wxTextEntryDialog( this, _( "Label Name: " ), _( "Enter the new label name" ), ( * m_Items )[ item ].m_Name );
        if( EntryDialog->ShowModal() == wxID_OK )
        {
            ( ( guDbRadios * ) m_Db )->SetRadioLabelName( Selection[ 0 ], EntryDialog->GetValue() );
            ReloadItems();
        }
        EntryDialog->Destroy();
    }
}

// -------------------------------------------------------------------------------- //
void guRadioLabelListBox::CreateAcceleratorTable( void )
{
    wxAcceleratorTable AccelTable;
    wxArrayInt AliasAccelCmds;
    wxArrayInt RealAccelCmds;

    AliasAccelCmds.Add( ID_PLAYER_PLAYLIST_SEARCH );

    RealAccelCmds.Add( ID_RADIO_SEARCH );

    if( guAccelDoAcceleratorTable( AliasAccelCmds, RealAccelCmds, AccelTable ) )
    {
        SetAcceleratorTable( AccelTable );
    }
}



// -------------------------------------------------------------------------------- //
// guRadioPanel
// -------------------------------------------------------------------------------- //
guRadioPanel::guRadioPanel( wxWindow * parent, guDbLibrary * db, guPlayerPanel * playerpanel ) :
                guAuiManagerPanel( parent ),
                m_TextChangedTimer( this, guRADIO_TIMER_TEXTSEARCH )
{
    m_Db = new guDbRadios( guPATH_RADIOS_DBNAME );
    m_PlayerPanel = playerpanel;
    m_RadioPlayListLoadThread = NULL;

    guConfig *  Config = ( guConfig * ) guConfig::Get();
    Config->RegisterObject( this );

    InitPanelData();

    m_VisiblePanels = Config->ReadNum( wxT( "VisiblePanels" ), guPANEL_RADIO_VISIBLE_DEFAULT, wxT( "radios" ) );
    m_InstantSearchEnabled = Config->ReadBool( wxT( "InstantTextSearchEnabled" ), true, wxT( "general" ) );
    m_EnterSelectSearchEnabled = !Config->ReadBool( wxT( "TextSearchEnterRelax" ), false, wxT( "general" ) );


	wxBoxSizer * SearchSizer;
	SearchSizer = new wxBoxSizer( wxHORIZONTAL );
    wxPanel * SearchPanel;
	SearchPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );

//    wxStaticText *      SearchStaticText;
//	SearchStaticText = new wxStaticText( SearchPanel, wxID_ANY, _( "Search:" ), wxDefaultPosition, wxDefaultSize, 0 );
//	SearchStaticText->Wrap( -1 );
//	SearchSizer->Add( SearchStaticText, 0, wxALIGN_CENTER|wxALL, 5 );

    m_InputTextCtrl = new wxSearchCtrl( SearchPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    SearchSizer->Add( m_InputTextCtrl, 1, wxALIGN_CENTER, 5 );

    SearchPanel->SetSizer( SearchSizer );
    SearchPanel->Layout();
	SearchSizer->Fit( SearchPanel );

    m_AuiManager.AddPane( SearchPanel,
            wxAuiPaneInfo().Name( wxT( "RadioTextSearch" ) ).Caption( _( "Text Search" ) ).
            Direction( 1 ).Layer( 2 ).Row( 0 ).Position( 0 ).BestSize( 111, 26 ).MinSize( 60, 26 ).MaxSize( -1, 26 ).
            CloseButton( Config->ReadBool( wxT( "ShowPaneCloseButton" ), true, wxT( "general" ) ) ).
            Dockable( true ).Top() );

    wxPanel * GenrePanel;
	GenrePanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer * GenreSizer;
	GenreSizer = new wxBoxSizer( wxVERTICAL );

    m_GenresTreeCtrl = new guRadioGenreTreeCtrl( GenrePanel, m_Db );
	GenreSizer->Add( m_GenresTreeCtrl, 1, wxEXPAND, 5 );

	GenrePanel->SetSizer( GenreSizer );
	GenrePanel->Layout();
	GenreSizer->Fit( GenrePanel );

	m_AuiManager.AddPane( GenrePanel,
            wxAuiPaneInfo().Name( wxT( "RadioGenres" ) ).Caption( _( "Genre" ) ).
            Direction( 4 ).Layer( 1 ).Row( 0 ).Position( 0 ).BestSize( 220, 60 ).MinSize( 60, 60 ).
            CloseButton( Config->ReadBool( wxT( "ShowPaneCloseButton" ), true, wxT( "general" ) ) ).
            Dockable( true ).Left() );

    wxPanel * LabelsPanel;
	LabelsPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer * LabelsSizer;
	LabelsSizer = new wxBoxSizer( wxVERTICAL );

	m_LabelsListBox = new guRadioLabelListBox( LabelsPanel, m_Db, _( "Labels" ) );
	LabelsSizer->Add( m_LabelsListBox, 1, wxEXPAND, 5 );

	LabelsPanel->SetSizer( LabelsSizer );
	LabelsPanel->Layout();
	LabelsSizer->Fit( LabelsPanel );

	m_AuiManager.AddPane( LabelsPanel,
            wxAuiPaneInfo().Name( wxT( "RadioLabels" ) ).Caption( _( "Labels" ) ).
            Direction( 4 ).Layer( 1 ).Row( 0 ).Position( 1 ).BestSize( 220, 60 ).MinSize( 60, 60 ).
            CloseButton( Config->ReadBool( wxT( "ShowPaneCloseButton" ), true, wxT( "general" ) ) ).
            Dockable( true ).Left() );


    wxPanel * StationsPanel;
	StationsPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );

	wxBoxSizer * StationsSizer;
	StationsSizer = new wxBoxSizer( wxVERTICAL );

	m_StationsListBox = new guRadioStationListBox( StationsPanel, m_Db );

	StationsSizer->Add( m_StationsListBox, 1, wxEXPAND, 5 );

	StationsPanel->SetSizer( StationsSizer );
	StationsPanel->Layout();
	StationsSizer->Fit( StationsPanel );

    m_AuiManager.AddPane( StationsPanel, wxAuiPaneInfo().Name( wxT( "RadioStations" ) ).Caption( _( "Stations" ) ).
            MinSize( 50, 50 ).
            CenterPane() );



    wxString RadioLayout = Config->ReadStr( wxT( "LastLayout" ), wxEmptyString, wxT( "radios" ) );
    if( Config->GetIgnoreLayouts() || RadioLayout.IsEmpty() )
    {
        m_VisiblePanels = guPANEL_RADIO_VISIBLE_DEFAULT;
        RadioLayout = wxT( "layout2|name=RadioTextSearch;caption=" ) + wxString( _( "Text Search" ) );
        RadioLayout += wxT( ";state=2099196;dir=1;layer=2;row=0;pos=0;prop=100000;bestw=111;besth=26;minw=60;minh=26;maxw=-1;maxh=26;floatx=-1;floaty=-1;floatw=-1;floath=-1|" );
        RadioLayout += wxT( "name=RadioGenres;caption=" ) + wxString( _( "Genres" ) ) + wxT( ";state=2099196;dir=4;layer=1;row=0;pos=0;prop=100000;bestw=220;besth=60;minw=60;minh=60;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|" );
        RadioLayout += wxT( "name=RadioLabels;caption=" ) + wxString( _( "Labels" ) ) + wxT( ";state=2099196;dir=4;layer=1;row=0;pos=1;prop=100000;bestw=220;besth=60;minw=60;minh=60;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|" );
        RadioLayout += wxT( "name=RadioStations;caption=" ) + wxString( _( "Stations" ) ) + wxT( ";state=768;dir=5;layer=0;row=0;pos=0;prop=100000;bestw=50;besth=50;minw=50;minh=50;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|" );
        RadioLayout += wxT( "dock_size(1,2,0)=47|dock_size(4,1,0)=179|dock_size(5,0,0)=52|" );
        //m_AuiManager.Update();
    }

    m_AuiManager.LoadPerspective( RadioLayout, true );


	Connect( guRADIO_TIMER_TEXTSEARCH, wxEVT_TIMER, wxTimerEventHandler( guRadioPanel::OnTextChangedTimer ), NULL, this );

    Connect( wxEVT_COMMAND_TREE_SEL_CHANGED,  wxTreeEventHandler( guRadioPanel::OnRadioGenreListSelected ), NULL, this );

    m_GenresTreeCtrl->Connect( wxEVT_COMMAND_TREE_ITEM_ACTIVATED, wxTreeEventHandler( guRadioPanel::OnRadioGenreListActivated ), NULL, this );

    m_LabelsListBox->Connect( wxEVT_COMMAND_LISTBOX_SELECTED,  wxListEventHandler( guRadioPanel::OnRadioLabelListSelected ), NULL, this );

    //
    Connect( ID_RADIO_USER_ADD, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioPanel::OnRadioUserAdd ), NULL, this );
    Connect( ID_RADIO_USER_EDIT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioPanel::OnRadioUserEdit ), NULL, this );
    Connect( ID_RADIO_USER_DEL, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioPanel::OnRadioUserDel ), NULL, this );

    Connect( ID_RADIO_SEARCH_ADD, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioPanel::OnRadioSearchAdd ), NULL, this );
    Connect( ID_RADIO_SEARCH_EDIT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioPanel::OnRadioSearchEdit ), NULL, this );
    Connect( ID_RADIO_SEARCH_DELETE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioPanel::OnRadioSearchDel ), NULL, this );

    Connect( ID_RADIO_USER_EXPORT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioPanel::OnRadioUserExport ), NULL, this );
    Connect( ID_RADIO_USER_IMPORT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioPanel::OnRadioUserImport ), NULL, this );

    Connect( ID_RADIO_DOUPDATE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioPanel::OnRadioUpdate ), NULL, this );
    Connect( ID_RADIO_UPDATED, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioPanel::OnRadioUpdated ), NULL, this );
    Connect( ID_RADIO_UPDATE_END, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioPanel::OnRadioUpdateEnd ), NULL, this );

    m_StationsListBox->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxListEventHandler( guRadioPanel::OnStationListActivated ), NULL, this );
	m_StationsListBox->Connect( wxEVT_COMMAND_LIST_COL_CLICK, wxListEventHandler( guRadioPanel::OnStationListBoxColClick ), NULL, this );
    Connect( ID_RADIO_EDIT_LABELS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioPanel::OnStationsEditLabelsClicked ) );

    m_InputTextCtrl->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( guRadioPanel::OnSearchSelected ), NULL, this );
    m_InputTextCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( guRadioPanel::OnSearchActivated ), NULL, this );
    //m_InputTextCtrl->Connect( wxEVT_COMMAND_SEARCHCTRL_SEARCH_BTN, wxCommandEventHandler( guRadioPanel::OnSearchActivated ), NULL, this );
    m_InputTextCtrl->Connect( wxEVT_COMMAND_SEARCHCTRL_CANCEL_BTN, wxCommandEventHandler( guRadioPanel::OnSearchCancelled ), NULL, this );

    Connect( ID_RADIO_PLAY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioPanel::OnRadioStationsPlay ), NULL, this );
    Connect( ID_RADIO_ENQUEUE_AFTER_ALL, ID_RADIO_ENQUEUE_AFTER_ARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioPanel::OnRadioStationsEnqueue ), NULL, this );

    Connect( ID_RADIO_PLAYLIST_LOADED, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioPanel::OnStationPlayListLoaded ), NULL, this );

    Connect( ID_RADIO_SEARCH, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioPanel::OnGoToSearch ), NULL, this );

    Connect( ID_CONFIG_UPDATED, guConfigUpdatedEvent, wxCommandEventHandler( guRadioPanel::OnConfigUpdated ), NULL, this );
}

// -------------------------------------------------------------------------------- //
guRadioPanel::~guRadioPanel()
{
    guConfig * Config = ( guConfig * ) guConfig::Get();
    Config->UnRegisterObject( this );

    Config->WriteNum( wxT( "VisiblePanels" ), m_VisiblePanels, wxT( "radios" ) );
    Config->WriteStr( wxT( "LastLayout" ), m_AuiManager.SavePerspective(), wxT( "radios" ) );

    m_RadioPlayListLoadThreadMutex.Lock();
    if( m_RadioPlayListLoadThread )
    {
        m_RadioPlayListLoadThread->Pause();
        m_RadioPlayListLoadThread->Delete();
    }
    m_RadioPlayListLoadThreadMutex.Unlock();

    //
	Disconnect( guRADIO_TIMER_TEXTSEARCH, wxEVT_TIMER, wxTimerEventHandler( guRadioPanel::OnTextChangedTimer ), NULL, this );

    Disconnect( wxEVT_COMMAND_TREE_SEL_CHANGED,  wxTreeEventHandler( guRadioPanel::OnRadioGenreListSelected ), NULL, this );

    m_GenresTreeCtrl->Disconnect( wxEVT_COMMAND_TREE_ITEM_ACTIVATED, wxTreeEventHandler( guRadioPanel::OnRadioGenreListActivated ), NULL, this );

    m_LabelsListBox->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED,  wxListEventHandler( guRadioPanel::OnRadioLabelListSelected ), NULL, this );

    //
    Disconnect( ID_RADIO_USER_ADD, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioPanel::OnRadioUserAdd ), NULL, this );
    Disconnect( ID_RADIO_USER_EDIT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioPanel::OnRadioUserEdit ), NULL, this );
    Disconnect( ID_RADIO_USER_DEL, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioPanel::OnRadioUserDel ), NULL, this );

    Disconnect( ID_RADIO_SEARCH_ADD, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioPanel::OnRadioSearchAdd ), NULL, this );
    Disconnect( ID_RADIO_SEARCH_EDIT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioPanel::OnRadioSearchEdit ), NULL, this );
    Disconnect( ID_RADIO_SEARCH_DELETE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioPanel::OnRadioSearchDel ), NULL, this );

    Disconnect( ID_RADIO_USER_EXPORT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioPanel::OnRadioUserExport ), NULL, this );
    Disconnect( ID_RADIO_USER_IMPORT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioPanel::OnRadioUserImport ), NULL, this );

    Disconnect( ID_RADIO_DOUPDATE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioPanel::OnRadioUpdate ), NULL, this );
    Disconnect( ID_RADIO_UPDATED, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioPanel::OnRadioUpdated ), NULL, this );
    Disconnect( ID_RADIO_UPDATE_END, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioPanel::OnRadioUpdateEnd ), NULL, this );

    m_StationsListBox->Disconnect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxListEventHandler( guRadioPanel::OnStationListActivated ), NULL, this );
	m_StationsListBox->Disconnect( wxEVT_COMMAND_LIST_COL_CLICK, wxListEventHandler( guRadioPanel::OnStationListBoxColClick ), NULL, this );
    Disconnect( ID_RADIO_EDIT_LABELS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioPanel::OnStationsEditLabelsClicked ) );

    m_InputTextCtrl->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( guRadioPanel::OnSearchSelected ), NULL, this );
    m_InputTextCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( guRadioPanel::OnSearchActivated ), NULL, this );
    //m_InputTextCtrl->Disconnect( wxEVT_COMMAND_SEARCHCTRL_SEARCH_BTN, wxCommandEventHandler( guRadioPanel::OnSearchActivated ), NULL, this );
    m_InputTextCtrl->Disconnect( wxEVT_COMMAND_SEARCHCTRL_CANCEL_BTN, wxCommandEventHandler( guRadioPanel::OnSearchCancelled ), NULL, this );

    Disconnect( ID_RADIO_PLAY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioPanel::OnRadioStationsPlay ), NULL, this );
    Disconnect( ID_RADIO_ENQUEUE_AFTER_ALL, ID_RADIO_ENQUEUE_AFTER_ARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioPanel::OnRadioStationsEnqueue ), NULL, this );

    Disconnect( ID_RADIO_PLAYLIST_LOADED, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guRadioPanel::OnStationPlayListLoaded ), NULL, this );

    if( m_Db )
    {
        delete m_Db;
    }
}

// -------------------------------------------------------------------------------- //
void guRadioPanel::InitPanelData( void )
{
    m_PanelNames.Add( wxT( "RadioTextSearch" ) );
    m_PanelNames.Add( wxT( "RadioLabels" ) );
    m_PanelNames.Add( wxT( "RadioGenres" ) );

    m_PanelIds.Add( guPANEL_RADIO_TEXTSEARCH );
    m_PanelIds.Add( guPANEL_RADIO_LABELS );
    m_PanelIds.Add( guPANEL_RADIO_GENRES );

    m_PanelCmdIds.Add( ID_MENU_VIEW_RAD_TEXTSEARCH );
    m_PanelCmdIds.Add( ID_MENU_VIEW_RAD_LABELS );
    m_PanelCmdIds.Add( ID_MENU_VIEW_RAD_GENRES );
}

// -------------------------------------------------------------------------------- //
void guRadioPanel::OnConfigUpdated( wxCommandEvent &event )
{
    int Flags = event.GetInt();
    if( Flags & guPREFERENCE_PAGE_FLAG_GENERAL )
    {
        guConfig * Config = ( guConfig * ) guConfig::Get();
        m_InstantSearchEnabled = Config->ReadBool( wxT( "InstantTextSearchEnabled" ), true, wxT( "general" ) );
        m_EnterSelectSearchEnabled = !Config->ReadBool( wxT( "TextSearchEnterRelax" ), false, wxT( "general" ) );
    }
}

// -------------------------------------------------------------------------------- //
void guRadioPanel::OnSearchActivated( wxCommandEvent& event )
{
    if( m_TextChangedTimer.IsRunning() )
        m_TextChangedTimer.Stop();

    if( !m_InstantSearchEnabled )
        return;

    m_TextChangedTimer.Start( guRADIO_TIMER_TEXTSEARCH_VALUE, wxTIMER_ONE_SHOT );
}

// -------------------------------------------------------------------------------- //
void guRadioPanel::OnSearchSelected( wxCommandEvent& event )
{
    guConfig * Config = ( guConfig * ) guConfig::Get();

    if( m_TextChangedTimer.IsRunning() )
        m_TextChangedTimer.Stop();

    if( !DoTextSearch() || !m_EnterSelectSearchEnabled || !m_InstantSearchEnabled )
        return;

    OnSelectStations( Config->ReadBool( wxT( "DefaultActionEnqueue" ), false, wxT( "general" ) ) );
}

// -------------------------------------------------------------------------------- //
void guRadioPanel::OnSearchCancelled( wxCommandEvent &event ) // CLEAN SEARCH STR
{
    m_InputTextCtrl->Clear();

    if( !m_InstantSearchEnabled )
        DoTextSearch();
}

// -------------------------------------------------------------------------------- //
bool guRadioPanel::DoTextSearch( void )
{
    wxString SearchString = m_InputTextCtrl->GetLineText( 0 );
    //guLogMessage( wxT( "Should do the search now: '%s'" ), SearchString.c_str() );
    if( !SearchString.IsEmpty() )
    {
        if( SearchString.Length() > 0 )
        {
            wxArrayString Words = guSplitWords( SearchString );
            m_Db->SetRaTeFilters( Words );
            m_LabelsListBox->ReloadItems();
            m_GenresTreeCtrl->ReloadItems();
            m_StationsListBox->ReloadItems();
        }

        m_InputTextCtrl->ShowCancelButton( true );
        return true;
    }
    else
    {
        wxArrayString Words;
        m_Db->SetRaTeFilters( Words );
        m_LabelsListBox->ReloadItems();
        m_GenresTreeCtrl->ReloadItems();
        m_StationsListBox->ReloadItems();
        m_InputTextCtrl->ShowCancelButton( false );
    }
    return false;
}

// -------------------------------------------------------------------------------- //
void guRadioPanel::OnTextChangedTimer( wxTimerEvent &event )
{
    DoTextSearch();
}

// -------------------------------------------------------------------------------- //
void guRadioPanel::OnStationListBoxColClick( wxListEvent &event )
{
    int ColId = m_StationsListBox->GetColumnId( event.m_col );
    m_StationsListBox->SetStationsOrder( ColId );
}


// -------------------------------------------------------------------------------- //
void guRadioPanel::OnStationsEditLabelsClicked( wxCommandEvent &event )
{
    guListItems Stations;
    m_StationsListBox->GetSelectedItems( &Stations );
    if( Stations.Count() )
    {
        wxArrayInt SCIds = m_Db->GetStationsSCIds( m_StationsListBox->GetSelectedItems() );
        guArrayListItems LabelSets = m_Db->GetStationsLabels( SCIds );

        guLabelEditor * LabelEditor = new guLabelEditor( this, ( guDbLibrary * ) m_Db, _( "Stations Labels Editor" ), true, &Stations, &LabelSets );
        if( LabelEditor )
        {
            if( LabelEditor->ShowModal() == wxID_OK )
            {
                // Update the labels in the files
                m_Db->SetRadioStationsLabels( LabelSets );
            }
            LabelEditor->Destroy();

            m_LabelsListBox->ReloadItems( false );
        }
    }
}

// -------------------------------------------------------------------------------- //
void guRadioPanel::OnStationListActivated( wxListEvent &event )
{
    guConfig * Config = ( guConfig * ) guConfig::Get();
    OnSelectStations( Config->ReadBool( wxT( "DefaultActionEnqueue" ), false, wxT( "general" ) ) );
}

// -------------------------------------------------------------------------------- //
void guRadioPanel::OnRadioGenreListSelected( wxTreeEvent &event )
{
    wxTreeItemId ItemId = event.GetItem();

    guShoutcastItemData * ItemData = ( guShoutcastItemData * ) m_GenresTreeCtrl->GetItemData( ItemId );
    if( ItemData )
    {
        //
        m_Db->SetRadioSourceFilter( ItemData->GetSource() );

        wxArrayInt RadioGenres;
        RadioGenres.Add( ItemData->GetId() );
        m_Db->SetRadioGenresFilters( RadioGenres );
    }
    else if( ItemId == * m_GenresTreeCtrl->GetShoutcastGenreId() )
    {
        m_Db->SetRadioSourceFilter( guRADIO_SOURCE_GENRE );
    }
    else if( ItemId == * m_GenresTreeCtrl->GetShoutcastSearchId() )
    {
        m_Db->SetRadioSourceFilter( guRADIO_SOURCE_SEARCH );
    }
    else if( ItemId == * m_GenresTreeCtrl->GetManualId() )
    {
        m_Db->SetRadioSourceFilter( guRADIO_SOURCE_USER );
    }
    else
    {
        m_Db->SetRadioSourceFilter( guRADIO_SOURCE_GENRE );
    }
    m_StationsListBox->ReloadItems();
}

// -------------------------------------------------------------------------------- //
void guRadioPanel::OnRadioGenreListActivated( wxTreeEvent &event )
{
    wxTreeItemId ItemId = event.GetItem();
    guShoutcastItemData * ItemData = ( guShoutcastItemData * ) m_GenresTreeCtrl->GetItemData( ItemId );
    if( ItemData )
    {
        wxArrayInt RadioGenres;
        RadioGenres.Add( ItemData->GetId() );

        guMainFrame * MainFrame = ( guMainFrame * ) wxTheApp->GetTopWindow();
        int GaugeId = ( ( guStatusBar * ) MainFrame->GetStatusBar() )->AddGauge( _( "Radios" ) );
        guUpdateRadiosThread * UpdateRadiosThread = new guUpdateRadiosThread( m_Db, this, RadioGenres, ItemData->GetSource(), GaugeId );
        if( !UpdateRadiosThread )
        {
            guLogError( wxT( "Could not create the radio update thread" ) );
        }
    }
    event.Skip();
}

// -------------------------------------------------------------------------------- //
void guRadioPanel::OnRadioLabelListSelected( wxListEvent &Event )
{
    m_Db->SetRadioLabelsFilters( m_LabelsListBox->GetSelectedItems() );
    m_GenresTreeCtrl->ReloadItems();
    m_StationsListBox->ReloadItems();
}

// -------------------------------------------------------------------------------- //
void guRadioPanel::OnRadioUpdate( wxCommandEvent &event )
{
    wxTreeItemId            ItemId;
    wxArrayInt              GenresIds;
    guShoutcastItemData *   ItemData;
    int Source = guRADIO_SOURCE_GENRE;

    wxSetCursor( * wxHOURGLASS_CURSOR );

    ItemId = m_GenresTreeCtrl->GetSelection();
    if( ItemId.IsOk() )
    {
        ItemData = ( guShoutcastItemData * ) m_GenresTreeCtrl->GetItemData( ItemId );
        if( ItemData )
        {
            Source = ItemData->GetSource();
            GenresIds.Add( ItemData->GetId() );
        }
        else if( ItemId == * m_GenresTreeCtrl->GetShoutcastSearchId() )
        {
            Source = guRADIO_SOURCE_SEARCH;
            guListItems Genres;
            m_Db->GetRadioGenres( guRADIO_SOURCE_SEARCH, &Genres );
            int index;
            int count = Genres.Count();
            for( index = 0; index < count; index++ )
            {
                GenresIds.Add( Genres[ index ].m_Id );
            }
        }
    }

    if( !GenresIds.Count() )
    {
        guListItems Genres;
        m_Db->GetRadioGenres( guRADIO_SOURCE_GENRE, &Genres );
        int index;
        int count = Genres.Count();
        for( index = 0; index < count; index++ )
        {
            GenresIds.Add( Genres[ index ].m_Id );
        }
    }

    if( GenresIds.Count() )
    {
        guMainFrame * MainFrame = ( guMainFrame * ) wxTheApp->GetTopWindow();
        int GaugeId = ( ( guStatusBar * ) MainFrame->GetStatusBar() )->AddGauge( _( "Radios" ) );
        guUpdateRadiosThread * UpdateRadiosThread = new guUpdateRadiosThread( m_Db, this, GenresIds, Source, GaugeId );
        if( !UpdateRadiosThread )
        {
            guLogError( wxT( "Could not create the radio update thread" ) );
        }
    }
    else
        wxSetCursor( wxNullCursor );
}

// -------------------------------------------------------------------------------- //
void guRadioPanel::OnRadioUpdated( wxCommandEvent &event )
{
    //GenresListBox->ReloadItems( false );
    m_StationsListBox->ReloadItems( false );
}

// -------------------------------------------------------------------------------- //
void guRadioPanel::OnRadioUpdateEnd( wxCommandEvent &event )
{
    wxSetCursor( wxNullCursor );
    //GenresListBox->Enable();
}

// -------------------------------------------------------------------------------- //
void guRadioPanel::OnRadioStationsPlay( wxCommandEvent &event )
{
    OnSelectStations( false );
}

// -------------------------------------------------------------------------------- //
void guRadioPanel::OnRadioStationsEnqueue( wxCommandEvent &event )
{
    OnSelectStations( true, event.GetId() - ID_RADIO_ENQUEUE_AFTER_ALL );
}

// -------------------------------------------------------------------------------- //
void guRadioPanel::OnSelectStations( bool enqueue, const int aftercurrent )
{
    wxString StationUrl;
    guRadioStation RadioStation;

    if( m_StationsListBox->GetSelected( &RadioStation ) )
    {
        if( RadioStation.m_SCId != wxNOT_FOUND )
        {
            guShoutCast ShoutCast;
            StationUrl = ShoutCast.GetStationUrl( RadioStation.m_SCId );
        }
        else
        {
            StationUrl = RadioStation.m_Link;
        }

        if( !StationUrl.IsEmpty() )
        {
            LoadStationUrl( StationUrl, enqueue, aftercurrent );
        }
    }
}

// -------------------------------------------------------------------------------- //
void guRadioPanel::LoadStationUrl( const wxString &stationurl, const bool enqueue, const int aftercurrent )
{
    m_RadioPlayListLoadThreadMutex.Lock();
    if( m_RadioPlayListLoadThread )
    {
        m_RadioPlayListLoadThread->Pause();
        m_RadioPlayListLoadThread->Delete();
    }

    m_StationPlayListTracks.Empty();
    m_RadioPlayListLoadThread = new guRadioPlayListLoadThread( this, stationurl.c_str(), &m_StationPlayListTracks, enqueue, aftercurrent );
    if( !m_RadioPlayListLoadThread )
    {
        guLogError( wxT( "Could not create the download radio playlist thread" ) );
    }
    m_RadioPlayListLoadThreadMutex.Unlock();
}

// -------------------------------------------------------------------------------- //
void guRadioPanel::OnStationPlayListLoaded( wxCommandEvent &event )
{
    bool Enqueue = event.GetInt();
    int  AfterCurrent = event.GetExtraLong();

    if( m_StationPlayListTracks.Count() )
    {
        if( Enqueue )
        {
            m_PlayerPanel->AddToPlayList( m_StationPlayListTracks, true, AfterCurrent );
        }
        else
        {
            m_PlayerPanel->SetPlayList( m_StationPlayListTracks );
        }
    }
    else
    {
        wxMessageBox( _( "There are not entries for this Radio Station" ) );
    }
}

// -------------------------------------------------------------------------------- //
void guRadioPanel::OnRadioUserAdd( wxCommandEvent &event )
{
    guRadioEditor * RadioEditor = new guRadioEditor( this, _( "Edit Radio" ) );
    if( RadioEditor )
    {
        if( RadioEditor->ShowModal() == wxID_OK )
        {
            guRadioStation RadioStation;
            RadioStation.m_Id = wxNOT_FOUND;
            RadioStation.m_SCId = wxNOT_FOUND;
            RadioStation.m_BitRate = 0;
            RadioStation.m_GenreId = wxNOT_FOUND;
            RadioStation.m_Source = 1;
            RadioStation.m_Name = RadioEditor->GetName();
            RadioStation.m_Link = RadioEditor->GetLink();
            RadioStation.m_Listeners = 0;
            RadioStation.m_Type = wxEmptyString;
            m_Db->SetRadioStation( &RadioStation );
            //
            m_StationsListBox->ReloadItems();
        }
        RadioEditor->Destroy();
    }
}

// -------------------------------------------------------------------------------- //
void guRadioPanel::OnRadioUserEdit( wxCommandEvent &event )
{
    guRadioStation RadioStation;
    m_StationsListBox->GetSelected( &RadioStation );

    guRadioEditor * RadioEditor = new guRadioEditor( this, _( "Edit Radio" ), RadioStation.m_Name, RadioStation.m_Link );
    if( RadioEditor )
    {
        if( RadioEditor->ShowModal() == wxID_OK )
        {
            RadioStation.m_Name = RadioEditor->GetName();
            RadioStation.m_Link = RadioEditor->GetLink();
            m_Db->SetRadioStation( &RadioStation );
            //
            m_StationsListBox->ReloadItems();
        }
        RadioEditor->Destroy();
    }
}

// -------------------------------------------------------------------------------- //
void guRadioPanel::OnRadioUserDel( wxCommandEvent &event )
{
    guRadioStation RadioStation;
    m_StationsListBox->GetSelected( &RadioStation );
    m_Db->DelRadioStation( RadioStation.m_Id );
    m_StationsListBox->ReloadItems();
}

// -------------------------------------------------------------------------------- //
void guRadioPanel::OnRadioSearchAdd( wxCommandEvent &event )
{
    guShoutcastItemData ShoutcastItem( 0, guRADIO_SOURCE_SEARCH, wxEmptyString, guRADIO_SEARCH_FLAG_DEFAULT );
    guShoutcastSearch * ShoutcastSearch = new guShoutcastSearch( this, &ShoutcastItem );
    if( ShoutcastSearch )
    {
        if( ShoutcastSearch->ShowModal() == wxID_OK )
        {
            int RadioSearchId = m_Db->AddRadioGenre( ShoutcastItem.GetName(), guRADIO_SOURCE_SEARCH, ShoutcastItem.GetFlags() );
            m_GenresTreeCtrl->ReloadItems();
            m_GenresTreeCtrl->Expand( m_GenresTreeCtrl->GetShoutcastSearchId() );
            wxTreeItemId ItemId = m_GenresTreeCtrl->GetItemId( m_GenresTreeCtrl->GetShoutcastSearchId(), RadioSearchId );
            if( ItemId.IsOk() )
            {
                m_GenresTreeCtrl->SelectItem( ItemId );
                OnRadioUpdate( event );
            }
        }
        ShoutcastSearch->Destroy();
    }
}

// -------------------------------------------------------------------------------- //
void guRadioPanel::OnRadioSearchEdit( wxCommandEvent &event )
{
    wxTreeItemId ItemId = m_GenresTreeCtrl->GetSelection();
    guShoutcastItemData * ItemData = ( guShoutcastItemData * ) m_GenresTreeCtrl->GetItemData( ItemId );
    if( ItemData && ItemData->GetSource() == guRADIO_SOURCE_SEARCH )
    {
        guShoutcastSearch * ShoutcastSearch = new guShoutcastSearch( this, ItemData );
        if( ShoutcastSearch )
        {
            if( ShoutcastSearch->ShowModal() == wxID_OK )
            {
                m_Db->SetRadioGenre( ItemData->GetId(), ItemData->GetName(), guRADIO_SOURCE_SEARCH, ItemData->GetFlags() );
                m_GenresTreeCtrl->ReloadItems();
            }
            ShoutcastSearch->Destroy();
        }
    }
}

// -------------------------------------------------------------------------------- //
void guRadioPanel::OnRadioSearchDel( wxCommandEvent &event )
{
    wxArrayTreeItemIds ItemIds;
    int index;
    int count;

    if( ( count = m_GenresTreeCtrl->GetSelections( ItemIds ) ) )
    {
        if( wxMessageBox( _( "Are you sure to delete the selected radio searches?" ),
                          _( "Confirm" ),
                          wxICON_QUESTION|wxYES_NO|wxNO_DEFAULT, this ) == wxYES )
        {
            guShoutcastItemData * RadioGenreData;
            for( index = 0; index < count; index++ )
            {
                RadioGenreData = ( guShoutcastItemData * ) m_GenresTreeCtrl->GetItemData( ItemIds[ index ] );
                if( RadioGenreData )
                {
                    m_Db->DelRadioGenre( RadioGenreData->GetId() );
                }
            }
            m_GenresTreeCtrl->ReloadItems();
        }
    }
}

// -------------------------------------------------------------------------------- //
void guRadioPanel::OnRadioUserExport( wxCommandEvent &event )
{
    guRadioStations UserStations;
    m_Db->GetUserRadioStations( &UserStations );
    int Index;
    int Count;
    if( ( Count = UserStations.Count() ) )
    {

        wxFileDialog * FileDialog = new wxFileDialog( this,
            wxT( "Select the output xml filename" ),
            wxGetHomeDir(),
            wxT( "RadioStations.xml" ),
            wxT( "*.xml;*.xml" ),
            wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

        if( FileDialog )
        {
            if( FileDialog->ShowModal() == wxID_OK )
            {
                wxXmlDocument OutXml;
                //OutXml.SetRoot(  );
                wxXmlNode * RootNode = new wxXmlNode( wxXML_ELEMENT_NODE, wxT( "RadioStations" ) );

                for( Index = 0; Index < Count; Index++ )
                {
                    //guLogMessage( wxT( "Adding %s" ), UserStations[ Index ].m_Name.c_str() );
                    wxXmlNode * RadioNode = new wxXmlNode( wxXML_ELEMENT_NODE, wxT( "RadioStation" ) );

                    wxXmlNode * RadioName = new wxXmlNode( wxXML_ELEMENT_NODE, wxT( "Name" ) );
                    wxXmlNode * RadioNameVal = new wxXmlNode( wxXML_TEXT_NODE, wxT( "Name" ), UserStations[ Index ].m_Name );
                    RadioName->AddChild( RadioNameVal );
                    RadioNode->AddChild( RadioName );

                    wxXmlNode * RadioUrl = new wxXmlNode( wxXML_ELEMENT_NODE, wxT( "Url" ) );
                    wxXmlNode * RadioUrlVal = new wxXmlNode( wxXML_TEXT_NODE, wxT( "Url" ), UserStations[ Index ].m_Link );
                    RadioUrl->AddChild( RadioUrlVal );
                    RadioNode->AddChild( RadioUrl );

                    RootNode->AddChild( RadioNode );
                }
                OutXml.SetRoot( RootNode );
                OutXml.Save( FileDialog->GetPath() );
            }
            FileDialog->Destroy();
        }
    }
}

// -------------------------------------------------------------------------------- //
void ReadXmlRadioStation( wxXmlNode * node, guRadioStation * station )
{
    while( node )
    {
        if( node->GetName() == wxT( "Name" ) )
        {
            station->m_Name = node->GetNodeContent();
        }
        else if( node->GetName() == wxT( "Url" ) )
        {
            station->m_Link = node->GetNodeContent();
        }
        node = node->GetNext();
    }
}

// -------------------------------------------------------------------------------- //
void ReadXmlRadioStations( wxXmlNode * node, guRadioStations * stations )
{
    while( node && node->GetName() == wxT( "RadioStation" ) )
    {
        guRadioStation * RadioStation = new guRadioStation();

        RadioStation->m_Id = wxNOT_FOUND;
        RadioStation->m_SCId = wxNOT_FOUND;
        RadioStation->m_BitRate = 0;
        RadioStation->m_GenreId = wxNOT_FOUND;
        RadioStation->m_Source = 1;
        RadioStation->m_Listeners = 0;
        RadioStation->m_Type = wxEmptyString;

        ReadXmlRadioStation( node->GetChildren(), RadioStation );

        if( !RadioStation->m_Name.IsEmpty() && !RadioStation->m_Link.IsEmpty() )
            stations->Add( RadioStation );
        else
            delete RadioStation;

        node = node->GetNext();
    }
}

// -------------------------------------------------------------------------------- //
void guRadioPanel::OnRadioUserImport( wxCommandEvent &event )
{
    guRadioStations UserStations;

    wxFileDialog * FileDialog = new wxFileDialog( this,
        wxT( "Select the xml file" ),
        wxGetHomeDir(),
        wxEmptyString,
        wxT( "*.xml;*.xml" ),
        wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( FileDialog )
    {
        if( FileDialog->ShowModal() == wxID_OK )
        {
            wxFileInputStream Ins( FileDialog->GetPath() );
            wxXmlDocument XmlDoc( Ins );
            wxXmlNode * XmlNode = XmlDoc.GetRoot();
            if( XmlNode && XmlNode->GetName() == wxT( "RadioStations" ) )
            {
                ReadXmlRadioStations( XmlNode->GetChildren(), &UserStations );
                int Index;
                int Count;
                if( ( Count = UserStations.Count() ) )
                {
                    for( Index = 0; Index < Count; Index++ )
                    {
                        m_Db->SetRadioStation( &UserStations[ Index ] );
                    }
                    //
                    m_StationsListBox->ReloadItems();
                }
            }
        }
        FileDialog->Destroy();
    }
}

// -------------------------------------------------------------------------------- //
void guRadioPanel::GetRadioCounter( wxLongLong * count )
{
    * count = m_StationsListBox->GetItemCount();
}

// -------------------------------------------------------------------------------- //
void guRadioPanel::OnGoToSearch( wxCommandEvent &event )
{
    if( !( m_VisiblePanels & guPANEL_RADIO_TEXTSEARCH ) )
    {
        ShowPanel( guPANEL_RADIO_TEXTSEARCH, true );
    }

    if( FindFocus() != m_InputTextCtrl )
        m_InputTextCtrl->SetFocus();
}

// -------------------------------------------------------------------------------- //
bool guRadioPanel::GetListViewColumnData( const int id, int * index, int * width, bool * enabled )
{
    return m_StationsListBox->GetColumnData( id, index, width, enabled );
}

// -------------------------------------------------------------------------------- //
bool guRadioPanel::SetListViewColumnData( const int id, const int index, const int width, const bool enabled, const bool refresh )
{
    return m_StationsListBox->SetColumnData( id, index, width, enabled, refresh );
}




// -------------------------------------------------------------------------------- //
// guRadioPlayListLoadThread
// -------------------------------------------------------------------------------- //
guRadioPlayListLoadThread::guRadioPlayListLoadThread( guRadioPanel * radiopanel,
        const wxChar * stationurl, guTrackArray * tracks, const bool enqueue, const int aftercurrent ) :
        m_StationUrl( stationurl )
{
    m_RadioPanel = radiopanel;
    //m_StationUrl = stationurl;
    m_Tracks = tracks;
    m_Enqueue = enqueue;
    m_AfterCurrent = aftercurrent;

    if( Create() == wxTHREAD_NO_ERROR )
    {
        SetPriority( WXTHREAD_DEFAULT_PRIORITY - 10 );
        Run();
    }
}


// -------------------------------------------------------------------------------- //
guRadioPlayListLoadThread::~guRadioPlayListLoadThread()
{
    if( !TestDestroy() )
    {
        m_RadioPanel->EndStationPlayListLoaded();
    }
}

// -------------------------------------------------------------------------------- //
guRadioPlayListLoadThread::ExitCode guRadioPlayListLoadThread::Entry()
{
    guTrack * NewSong;
    guPlaylistFile PlayListFile;

    if( TestDestroy() )
        return 0;

    PlayListFile.Load( m_StationUrl );

    int Index;
    int Count;
    Count = PlayListFile.Count();
    for( Index = 0; Index < Count; Index++ )
    {
        if( TestDestroy() )
            break;

        NewSong = new guTrack();
        if( NewSong )
        {
            guPlaylistItem PlayListItem = PlayListFile.GetItem( Index );
            NewSong->m_Type = guTRACK_TYPE_RADIOSTATION;
            NewSong->m_FileName = PlayListItem.m_Location;
            //NewSong->m_SongName = PlayList[ index ].m_Name;
            NewSong->m_SongName = PlayListItem.m_Name;
            //NewSong->m_AlbumName = RadioStation.m_Name;
            NewSong->m_Length = 0;
            NewSong->m_Rating = -1;
            NewSong->m_Bitrate = 0;
            //NewSong->CoverId = guPLAYLIST_RADIOSTATION;
            NewSong->m_CoverId = 0;
            NewSong->m_Year = 0;
            m_Tracks->Add( NewSong );
        }
    }


    if( !TestDestroy() )
    {
        //m_RadioPanel->StationUrlLoaded( Tracks, m_Enqueue, m_AsNext );
        //guLogMessage( wxT( "Send Event for station '%s'" ), m_StationUrl.c_str() );
        wxCommandEvent Event( wxEVT_COMMAND_MENU_SELECTED, ID_RADIO_PLAYLIST_LOADED );
        Event.SetInt( m_Enqueue );
        Event.SetExtraLong( m_AfterCurrent );
        wxPostEvent( m_RadioPanel, Event );
    }

    return 0;
}

// -------------------------------------------------------------------------------- //
// guUpdateRadiosThread
// -------------------------------------------------------------------------------- //
bool inline guListItemsSearchName( guListItems &items, const wxString &name )
{
    int Index;
    int Count = items.Count();
    for( Index = 0; Index < Count; Index++ )
    {
        if( items[ Index ].m_Name.Lower() == name )
        {
            return true;
        }
    }
    return false;
}

// -------------------------------------------------------------------------------- //
guUpdateRadiosThread::guUpdateRadiosThread( guDbRadios * db, guRadioPanel * radiopanel,
                                const wxArrayInt &ids, const int source, int gaugeid )
{
    m_Db = db;
    m_RadioPanel = radiopanel;
    m_Ids = ids;
    m_Source = source;
    m_GaugeId = gaugeid;

    if( Create() == wxTHREAD_NO_ERROR )
    {
        SetPriority( WXTHREAD_DEFAULT_PRIORITY - 30 );
        Run();
    }
}


// -------------------------------------------------------------------------------- //
void guUpdateRadiosThread::CheckRadioStationsFilters( const int flags, const wxString &text, guRadioStations &stations )
{
    guListItems RadioGenres;
    m_Db->GetRadioGenres( guRADIO_SOURCE_GENRE, &RadioGenres );

    int Index;
    int Count = stations.Count();
    if( Count )
    {
        for( Index = Count - 1; Index >= 0; Index-- )
        {
            if( ( flags & guRADIO_SEARCH_FLAG_NOWPLAYING ) )
            {
                if( stations[ Index ].m_NowPlaying.Lower().Find( text ) == wxNOT_FOUND )
                {
                    stations.RemoveAt( Index );
                    continue;
                }
            }

            if( ( flags & guRADIO_SEARCH_FLAG_GENRE ) )
            {
                if( stations[ Index ].m_GenreName.Lower().Find( text ) == wxNOT_FOUND )
                {
                    stations.RemoveAt( Index );
                    continue;
                }
            }

            if( ( flags & guRADIO_SEARCH_FLAG_STATION ) )
            {
                if( stations[ Index ].m_Name.Lower().Find( text ) == wxNOT_FOUND )
                {
                    stations.RemoveAt( Index );
                    continue;
                }
            }

            // Need to check if the station genre already existed
            if( !( flags & guRADIO_SEARCH_FLAG_ALLGENRES ) )
            {
                if( !guListItemsSearchName( RadioGenres, stations[ Index ].m_GenreName.Lower() ) )
                {
                    stations.RemoveAt( Index );
                }
            }
        }
    }
}

// -------------------------------------------------------------------------------- //
guUpdateRadiosThread::ExitCode guUpdateRadiosThread::Entry()
{
//    guListItems Genres;
    int index;
    int count;
    wxCommandEvent event( wxEVT_COMMAND_MENU_SELECTED, ID_STATUSBAR_GAUGE_SETMAX );
    guShoutCast * ShoutCast = new guShoutCast();
    guRadioStations RadioStations;
    if( ShoutCast )
    {
        //
        guConfig * Config = ( guConfig * ) guConfig::Get();
        long MinBitRate;
        Config->ReadStr( wxT( "RadioMinBitRate" ), wxT( "128" ), wxT( "radios" ) ).ToLong( &MinBitRate );

//        m_Db->GetRadioGenres( &Genres, false );
//        guLogMessage( wxT ( "Loaded the genres" ) );
        guListItems Genres;
        wxArrayInt  Flags;
        m_Db->GetRadioGenresList( m_Source, m_Ids, &Genres, &Flags );

        //
        m_Db->DelRadioStations( m_Source, m_Ids );
        //guLogMessage( wxT( "Deleted all radio stations" ) );
        count = Genres.Count();

        event.SetInt( m_GaugeId );
        event.SetExtraLong( count );
        wxPostEvent( wxTheApp->GetTopWindow(), event );

        for( index = 0; index < count; index++ )
        {
            guLogMessage( wxT( "Updating radiostations for genre '%s'" ), Genres[ index ].m_Name.c_str() );
            ShoutCast->GetStations( m_Source, Flags[ index ], Genres[ index ].m_Name, Genres[ index ].m_Id, &RadioStations, MinBitRate );

            if( m_Source == guRADIO_SOURCE_SEARCH )
            {
                CheckRadioStationsFilters( Flags[ index ], Genres[ index ].m_Name.Lower(), RadioStations );
            }

            m_Db->SetRadioStations( &RadioStations );

            RadioStations.Clear();

            //wxCommandEvent event( wxEVT_COMMAND_MENU_SELECTED, ID_RADIO_UPDATED );
            event.SetId( ID_RADIO_UPDATED );
            wxPostEvent( m_RadioPanel, event );
            Sleep( 30 ); // Its wxThread::Sleep

//            wxCommandEvent event2( wxEVT_COMMAND_MENU_SELECTED, ID_STATUSBAR_GAUGE_UPDATE );
            event.SetId( ID_STATUSBAR_GAUGE_UPDATE );
            event.SetInt( m_GaugeId );
            event.SetExtraLong( index );
            wxPostEvent( wxTheApp->GetTopWindow(), event );
        }

        delete ShoutCast;
    }
//    wxCommandEvent event( wxEVT_COMMAND_MENU_SELECTED, ID_RADIO_UPDATE_END );
    event.SetId( ID_RADIO_UPDATE_END );
    wxPostEvent( m_RadioPanel, event );
//    wxMilliSleep( 1 );

//    wxCommandEvent event2( wxEVT_COMMAND_MENU_SELECTED, ID_STATUSBAR_GAUGE_REMOVE );
    event.SetId( ID_STATUSBAR_GAUGE_REMOVE );
    event.SetInt( m_GaugeId );
    wxPostEvent( wxTheApp->GetTopWindow(), event );
    //
    return 0;
}

// -------------------------------------------------------------------------------- //
