<?xml version="1.0" encoding="ISO-8859-1"?>
<!--
Name:      lexers.xsl
Purpose:   Stylesheet for lexers.xml
Author:    Anton van Wezenbeek
Created:   2009-08-13
RCS-ID:    $Id$
Copyright: (c) 2009 Anton van Wezenbeek
-->

<xsl:stylesheet version="1.0"
xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:template match="/">
<html>
<body>

<table border="1">
<tr>
<td>
    <h2>global</h2>
    <table border="1">
    <tr bgcolor="#9acd32">
      <th align="left">marker no</th>
      <th align="left">value</th>
    </tr>
    
    <xsl:for-each select="lexers/global/marker">
    <tr>
      <td><xsl:value-of select="@no"/></td>
      <td><xsl:value-of select="."/></td>
    </tr>
    </xsl:for-each>
    
    <tr bgcolor="#9acd32">
      <th align="left">style no</th>
      <th align="left">value</th>
    </tr>
    
    <xsl:for-each select="lexers/global/style">
    <tr>
      <td><xsl:value-of select="@no"/></td>
      <td><xsl:value-of select="."/></td>
    </tr>
    </xsl:for-each>
    
    <tr bgcolor="#9acd32">
      <th align="left">hex no</th>
      <th align="left">value</th>
    </tr>
    
    <xsl:for-each select="lexers/global/hex">
    <tr>
      <td><xsl:value-of select="@no"/></td>
      <td><xsl:value-of select="."/></td>
    </tr>
    </xsl:for-each>
    
    <tr bgcolor="#9acd32">
      <th align="left">indicator no</th>
      <th align="left">value</th>
    </tr>
    
    <xsl:for-each select="lexers/global/indicator">
    <tr>
      <td><xsl:value-of select="@no"/></td>
      <td><xsl:value-of select="."/></td>
    </tr>
    </xsl:for-each>
    
    </table>
</td>
</tr>
  
<tr>
<td>
    <h2>lexers</h2>
    <table border="1">
    <tr bgcolor="#9acd32">
      <th align="left">name</th>
      <th align="left">extensions</th>
      <th align="left">colourings</th>
      <th align="left">properties</th>
      <th align="left">comments</th>
      <th align="left">keywords</th>
    </tr>
    
    <xsl:for-each select="lexers/lexer">
    <tr>
      <td><xsl:value-of select="@name"/></td>
      <td><xsl:value-of select="@extensions"/></td>
      <td><xsl:value-of select="colourings"/></td>
      <td><xsl:value-of select="properties"/></td>
      <td><xsl:value-of select="comments"/></td>
      <td><xsl:value-of select="keywords"/></td>
    </tr>
    </xsl:for-each>
    
    </table>
</td>
</tr>
  
</table>

</body>
</html>
</xsl:template>
</xsl:stylesheet>
