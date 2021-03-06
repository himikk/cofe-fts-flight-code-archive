﻿
Public Class frmOptions

    Private Class Device
        Public name As String
        Public guid As String
        Public deviceIndex As Integer
        Public channels As New List(Of String)

        Public selectedChannels As New List(Of String)
    End Class

    ' Read the file
    ' Any line that starts with a semi-colon can be ignored entirely
    ' The data elements are embedded in brackets

    Private Function ReadOutputFile(ByVal filename As String) As List(Of String)
        Dim retv As New List(Of String)

        Dim LineReader As New Microsoft.VisualBasic.FileIO.TextFieldParser(filename)
        LineReader.TextFieldType = FileIO.FieldType.Delimited
        LineReader.SetDelimiters("[", "]")
        Dim currentRow As String()

        While Not LineReader.EndOfData
            Try
                currentRow = LineReader.ReadFields()
                For Each currentField As String In currentRow
                    If currentField.StartsWith(";") Then
                        Exit For
                    End If

                    If (currentField.Length > 0) Then
                        Debug.Print(currentField)
                        retv.Add(currentField)
                    End If
                Next
            Catch ex As  _
            Microsoft.VisualBasic.FileIO.MalformedLineException
                MsgBox("Line " & ex.Message & _
                "is not valid and will be skipped.")
            End Try
        End While

        Return retv
    End Function

    Private Function ReadDevice(ByRef parameters As List(Of String), ByVal index As Integer, ByRef newDevice As Device) As Integer
        ' First three parameters are the deviceName, deviceGuid, deviceIndex, channelCount
        ' each device must present at least 1 channel (and, by default, will really present at least three)
        If index + 5 > parameters.Count Then
            Throw New SystemException("Malformed Config file - insufficient data for device")
        End If

        newDevice.name = parameters(index)
        newDevice.guid = parameters(index + 1)
        newDevice.deviceIndex = Conversion.Int(Conversion.Val(parameters(index + 2)))
        Dim channelCount As Integer = Conversion.Int(Conversion.Val(parameters(index + 3)))

        index += 4

        If index + channelCount > parameters.Count Then
            Throw New SystemException("Malformed Config File - channel data is missing")
        End If

        newDevice.channels = New List(Of String)

        For i As Integer = 1 To channelCount
            newDevice.channels.Add(parameters(index + i - 1))
        Next

        index += channelCount

        Return index
    End Function

    Private Function ParseInputFile(ByVal filename As String) As List(Of Device)
        Dim retv As New List(Of Device)
        Dim parameters As List(Of String) = ReadOutputFile(filename)
        If (parameters.Count < 2) Then
            MsgBox("No devices found in " & filename)
            Return retv
        End If

        ' First parameter should be the count of devices
        Dim DevCount As Integer = Conversion.Int(Conversion.Val(parameters.ElementAt(0)))

        ' We need to keep reading devices as long as there is data
        ' start with the next field
        Dim index As Integer = 1

        Try
            While index < parameters.Count
                Dim newDevice As New Device
                index = ReadDevice(parameters, index, newDevice)
                retv.Add(newDevice)
            End While
        Catch ex As Exception
            MsgBox("Malformed configuration file, nothing will be done")
        End Try

        Return retv
    End Function

    Private Sub PopulateTree(ByVal filename As String)
        tvDevices.Nodes.Clear()

        Dim devices As List(Of Device) = ParseInputFile(filename)
        For Each source As Device In devices
            Dim node As TreeNode = tvDevices.Nodes.Add(source.name)
            For Each channel As String In source.channels
                node.Nodes.Add(channel).Checked = True
            Next
            node.Checked = True
            node.Tag = source
            node.Expand()
        Next
    End Sub


    Private Sub Form1_Load(ByVal sender As Object, ByVal e As System.EventArgs) Handles Me.Load
        CancelButton = btnCancel
        AcceptButton = btnOkay

        'PopulateTree("deviceInfo.txt")

    End Sub

    Private Sub UpdateParentCheck(ByRef changedNode As TreeNode)
        If changedNode.Parent Is Nothing Then
            Exit Sub
        End If

        ' If we have a check, then the parent MUST be active
        If changedNode.Checked = True Then
            changedNode.Parent.Checked = True
            Exit Sub
        End If

        ' If all the nodes are unchecked, then uncheck the parent
        Dim checkParent As Boolean = False
        For Each childNode As TreeNode In changedNode.Parent.Nodes
            If childNode.Checked = True Then
                checkParent = True
                Exit For
            End If
        Next

        changedNode.Parent.Checked = checkParent

    End Sub

    ' Updates all child tree nodes recursively.
    Private Sub CheckAllChildNodes(ByVal treeNode As TreeNode, ByVal nodeChecked As Boolean)
        Dim node As TreeNode
        For Each node In treeNode.Nodes
            node.Checked = nodeChecked
            If node.Nodes.Count > 0 Then
                ' If the current node has child nodes, call the CheckAllChildsNodes method recursively.
                Me.CheckAllChildNodes(node, nodeChecked)
            End If
        Next node
    End Sub

    Private Sub tvDevices_AfterCheck(ByVal sender As Object, ByVal e As System.Windows.Forms.TreeViewEventArgs) Handles tvDevices.AfterCheck
        If e.Action <> TreeViewAction.Unknown Then
            If e.Node.Nodes.Count > 0 Then
                ' Calls the CheckAllChildNodes method, passing in the current 
                ' Checked value of the TreeNode whose checked state changed. 
                Me.CheckAllChildNodes(e.Node, e.Node.Checked)
            End If

            UpdateParentCheck(e.Node)

            ' if we are selecting a child node, then the parent must be selected
            If e.Node.Checked AndAlso e.Node.Parent IsNot Nothing Then
                e.Node.Parent.Checked = True
            End If

        End If
    End Sub

    Private Sub btnAll_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btnAll.Click
        For Each node As TreeNode In tvDevices.Nodes
            node.Checked = True
            CheckAllChildNodes(node, node.Checked)
        Next
    End Sub

    Private Sub btnNone_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btnNone.Click
        For Each node As TreeNode In tvDevices.Nodes
            node.Checked = False
            CheckAllChildNodes(node, node.Checked)
        Next
    End Sub

    Private Sub SetChannelNameChecked(ByVal name As String, ByVal checked As Boolean)
        For Each root As TreeNode In tvDevices.Nodes
            For Each node As TreeNode In root.Nodes
                If node.Text = name Then
                    node.Checked = checked
                    UpdateParentCheck(node)
                    Exit For
                End If
            Next
        Next
    End Sub

    Private Sub btnTimerOn_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btnTimerOn.Click
        SetChannelNameChecked("implicitTimer", True)
    End Sub

    Private Sub btnTimerOff_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btnTimerOff.Click
        SetChannelNameChecked("implicitTimer", False)
    End Sub

    Private Sub btnIndexOn_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btnIndexOn.Click
        SetChannelNameChecked("index", True)
    End Sub

    Private Sub btnIndexOff_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btnIndexOff.Click
        SetChannelNameChecked("index", False)
    End Sub

    Private Sub btnCancel_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btnCancel.Click
        Close()
    End Sub

    Private Sub AddOutputParameter(ByRef outputData As List(Of String), ByVal parameter As Object)
        outputData.Add("[" & parameter.ToString & "]")
    End Sub

    Private Sub btnOkay_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btnOkay.Click
        ' Write the data and then close

        Dim outputDevices As New List(Of Device)
        For Each node As TreeNode In tvDevices.Nodes
            If node.Checked Then
                Dim d As Device = node.Tag
                Debug.Print(d.name)
                For Each channel As TreeNode In node.Nodes
                    If channel.Checked Then
                        d.selectedChannels.Add(channel.Text)
                        Debug.Print("    " & channel.Text)
                    End If
                Next

                outputDevices.Add(d)
            End If
        Next

        If outputDevices.Count() < 1 Then
            MsgBox("No output devices selected, Click Cancel, not Okay")
            Exit Sub
        End If

        ' create the output data
        Dim outputData As New List(Of String)
        outputData.Add("Found " & outputDevices.Count() & " devices" & ControlChars.CrLf)
        AddOutputParameter(outputData, outputDevices.Count)
        For Each dev As Device In outputDevices
            AddOutputParameter(outputData, dev.name)
            AddOutputParameter(outputData, dev.guid)
            AddOutputParameter(outputData, dev.deviceIndex)
            AddOutputParameter(outputData, dev.selectedChannels.Count())
            outputData.Add(ControlChars.CrLf)
            For Each channel As String In dev.selectedChannels
                AddOutputParameter(outputData, channel)
            Next
            outputData.Add(ControlChars.CrLf)
        Next

        Close()
    End Sub
End Class
