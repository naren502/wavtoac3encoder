﻿#pragma once

#include <afxcmn.h>
#include <afxwin.h>
#include "controls\MyDialogEx.h"
#include "controls\MyEdit.h"
#include "controls\MyListCtrl.h"
#include "configuration\Configuration.h"

class CEnginesDlg : public CMyDialogEx
{
    DECLARE_DYNAMIC(CEnginesDlg)
public:
    CEnginesDlg(CWnd* pParent = nullptr);
    virtual ~CEnginesDlg();
    enum { IDD = IDD_DIALOG_ENGINES };
protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    DECLARE_MESSAGE_MAP()
public:
    ConfigList m_EngineList;
    CString szCurrentFileName;
public:
    volatile bool bUpdateList;
public:
    CMyListCtrl m_LstEngines;
    CMyEdit m_EdtEngineName;
    CMyEdit m_EdtEnginePath;
    int nCurrSel;
public:
    bool InsertProgramEngines();
    bool LoadProgramEngines(CString szFileName);
    bool SaveProgramEngines(CString szFileName);
protected:
    void InitLang();
public:
    virtual BOOL OnInitDialog();
public:
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedCancel();
    afx_msg void OnBnClickedButtonEnginesBrowse();
    afx_msg void OnBnClickedButtonEnginesImport();
    afx_msg void OnBnClickedButtonEnginesExport();
    afx_msg void OnBnClickedButtonEnginesAdd();
    afx_msg void OnBnClickedButtonEnginesRemove();
    afx_msg void OnLvnItemchangedListEngines(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnEnChangeEditEngineName();
    afx_msg void OnEnChangeEditEnginePath();
    afx_msg void OnLvnKeydownListEngines(NMHDR *pNMHDR, LRESULT *pResult);
};
